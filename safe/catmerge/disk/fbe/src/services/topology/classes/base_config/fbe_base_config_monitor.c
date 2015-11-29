/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_config_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the base config object lifecycle code.
 * 
 *  This includes the
 *  @ref fbe_base_config_monitor_entry "base_config monitor entry point", as well as all
 *  the lifecycle defines such as rotaries and conditions, along with the actual condition
 *  functions.
 * 
 * @ingroup base_config_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe_base_object.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_config_private.h"
#include "fbe_cmi.h"
#include "fbe_database.h"
#include "fbe_transport_memory.h"
#include "fbe_private_space_layout.h"


/*!***************************************************************
 * fbe_base_config_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the base config's monitor.
 *
 * @param object_handle - This is the object handle.
 * @param packet_p - The packet arriving from the monitor scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.             
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_base_config_monitor_entry(fbe_object_handle_t object_handle, 
                              fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_base_config_t * base_config_p = NULL;

    base_config_p = (fbe_base_config_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_lifecycle_crank_object(&fbe_base_config_lifecycle_const, 
                                        (fbe_base_object_t*)base_config_p, packet_p);
    return status;
}
/******************************************
 * end fbe_base_config_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_base_config_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the base config.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the base config's constant
 *                        lifecycle data.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_base_config_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(base_config));
}
/******************************************
 * end fbe_base_config_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t base_config_detach_block_edges_cond_function(fbe_base_object_t * object_p, 
                                                 fbe_packet_t * packet_p);
static fbe_status_t base_config_detach_block_edges_cond_completion(fbe_packet_t * packet_p, 
                                                            fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t fbe_base_config_pending_func(fbe_base_object_t *base_object_p, 
                                                            fbe_packet_t * packet);

static fbe_lifecycle_status_t base_config_unregister_metadata_element_cond_function(fbe_base_object_t * object_p, 
                                                                           fbe_packet_t * packet_p);

static fbe_status_t base_config_unregister_metadata_element_cond_function_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t base_config_traffic_load_change_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t base_config_start_hibernation_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_config_dequeue_pending_io_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_config_wake_up_hibernating_servers_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_config_enable_power_saving_on_servers_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_config_check_power_save_disabled_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_config_reset_hibernation_timer_cond_function( fbe_base_object_t* in_object_p,fbe_packet_t* in_packet_p);
static fbe_lifecycle_status_t base_config_set_not_saving_power_cond_function( fbe_base_object_t* in_object_p,fbe_packet_t* in_packet_p  );
static fbe_lifecycle_status_t base_config_wait_for_server_wakeup_cond_function( fbe_base_object_t* in_object_p,fbe_packet_t* in_packet_p  );

static fbe_lifecycle_status_t base_config_update_metadata_memory_cond_function( fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t base_config_update_metadata_memory_cond_function_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t base_config_update_last_io_time_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_status_t base_config_update_last_io_time_cond_function_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t base_config_persist_non_paged_metadata_cond_function_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t base_config_clustered_activate_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_base_config_clustered_activate_request(fbe_base_config_t *  base_config_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_base_config_clustered_activate_started(fbe_base_config_t *  base_config_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t base_config_check_nonpaged_metadata_version_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);


/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(base_config);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(base_config);

/*  base_config_lifecycle_callbacks
 *  This variable has all the callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(base_config) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        base_config,
        FBE_LIFECYCLE_NULL_FUNC,          /* online function */
        fbe_base_config_pending_func)   /* pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/

/*--- constant base condition entries --------------------------------------------------*/

/* FBE_BASE_CONFIG_LIFECYCLE_COND_DETACH_BLOCK_EDGES condition 
 *   The purpose of this condition is to detach the block transport edges.
*/
static fbe_lifecycle_const_base_cond_t base_config_detach_block_edges_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_detach_block_edge_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_DETACH_BLOCK_EDGES,
        base_config_detach_block_edges_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_BASE_CONFIG_LIFECYCLE_COND_UNREGISTER_METADATA_ELEMENT condition 
 *   The purpose of this condition is to unregister from the metadata service.
*/
static fbe_lifecycle_const_base_cond_t base_config_unregister_metadata_element_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_unregister_metadata_element_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_UNREGISTER_METADATA_ELEMENT,
        base_config_unregister_metadata_element_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* base_config_downstream_health_valid_for_specialize_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_NOT_OPTIMAL condition. 
 * The purpose of this condition is to hold the object until the 
 * downstream health is valid enough for us to proceed through specialze.
 *  
 * It is virtual condition which will be derived by the class which is inherited from the base config class.
 */
static fbe_lifecycle_const_base_cond_t base_config_downstream_health_not_optimal_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_downstream_health_not_optimal_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_NOT_OPTIMAL,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* base_config_downstream_health_broken_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_PATH_STATE_BROKEN condition.
 * The purpose of this condition is to take different action based on
 * its derived object type and edges state transition with downstream
 * objects.
 * It is virtual condition which will be derived by the class which is 
 * inherited from the base config class.
 */
static fbe_lifecycle_const_base_cond_t base_config_downstream_health_broken_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_downstream_health_broken_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_FAIL,               /* ACTIVATE */
        FBE_LIFECYCLE_STATE_FAIL,               /* READY */
        FBE_LIFECYCLE_STATE_FAIL,               /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* base_config_downstream_health_broken_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_PATH_STATE_DISABLED condition.
 * The purpose of this condition is to take different action based on
 * its derived object type and edges state transition with downstream
 * objects.
 * It is virtual condition which will be derived by the class which is 
 * inherited from the base config class.
 */
static fbe_lifecycle_const_base_cond_t base_config_downstream_health_disabled_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_downstream_health_disabled_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* READY */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* base_config_nonpaged_metadata_init_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_NONPAGED_METADATA_INIT condition.
 * The purpose of this condition is to initialize object's nonpaged metadata in memory.
 * It is virtual condition which will be derived by the class which is inherited from the base
 * config class.
 */
static fbe_lifecycle_const_base_cond_t base_config_nonpaged_metadata_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_nonpaged_metadata_init_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_NONPAGED_METADATA_INIT,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* base_config_metadata_memory_init_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_MEMORY_INIT condition.
 * The purpose of this condition is to initialize object metadata.
 * It is virtual condition which will be derived by the class which is 
 * inherited from the base config class.
 */
static fbe_lifecycle_const_base_cond_t base_config_metadata_memory_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_metadata_memory_init_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_MEMORY_INIT,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,             /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* base_config_zeroing_consumed_space_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_ZERO_CONSUMED_AREA condition.
 * The purpose of this condition is to send the zero request for the space which base config
 * object consumes for its operation.
 * It is virtual condition which will be derived by the class which is 
 * inherited from the base config class.
 */
static fbe_lifecycle_const_base_cond_t base_config_zero_consumed_area_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_zero_consumed_space_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_ZERO_CONSUMED_AREA,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,             /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,               /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


/* base_config_write_default_nonpaged_metadata_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_NONPAGED_METADATA condition.
 * The purpose of this condition is to write the object's nonpaged metadata with default values.
 * It is virtual condition which will be derived by the class which is inherited from the base
 * config class.
 */
static fbe_lifecycle_const_base_cond_t base_config_write_default_nonpaged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_write_default_nonpaged_metadata_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_NONPAGED_METADATA,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,             /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* base_config_write_default_nonpaged_metadata_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_INITIAL_PERSIST_NON_PAGED_METADATA condition.
 * The purpose of this condition is to persist the object's nonpaged metadata with default values.
 * It is virtual condition which will be derived by the class which is inherited from the base
 * config class.
 */
static fbe_lifecycle_const_base_cond_t base_config_persist_default_nonpaged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_persist_default_nonpaged_metadata_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_PERSIST_DEFAULT_NONPAGED_METADATA,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* base_config_paged_metadata_init_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_PAGED_METADATA condition.
 * The purpose of this condition is to write the object's default paged metadata.
 * It is virtual condition which will be derived by the class which is inherited from the base
 * config class.
 */
static fbe_lifecycle_const_base_cond_t base_config_write_default_paged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_write_default_paged_metadata_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_PAGED_METADATA,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,             /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


/* base_config_metadata_verify_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_SIGNATURE_VERIFY condition.
 * The purpose of this condition is to verify or initialize object metadata.
 * It is virtual condition which will be derived by the class which is 
 * inherited from the base config class.
 */
static fbe_lifecycle_const_base_cond_t base_config_metadata_verify_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_metadata_verify_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_VERIFY,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* base_config_metadata_element_init_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_ELEMENT_INIT condition.
 * The purpose of this condition is to initialize object metadata.
 * It is virtual condition which will be derived by the class which is 
 * inherited from the base config class.
 */
static fbe_lifecycle_const_base_cond_t base_config_metadata_element_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_metadata_element_init_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_ELEMENT_INIT,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};
/* base_config_negotiate_block_size_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_NEGOTIATE_BLOCK_SIZE condition. 
 * The purpose of this condition is to negotiate for the block size to be used. 
 * This also allows the object to get the capacity in the block size that will be used. 
 * Since this is not defined here, the subclass must define this.                         
 */
static fbe_lifecycle_const_base_cond_t base_config_negotiate_block_size_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_negotiate_block_size_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_NEGOTIATE_BLOCK_SIZE,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_BASE_CONFIG_LIFECYCLE_COND_ESTABLISH_EDGE_STATES -
 * 
 * This condition is used in the specialize state to wait for the edges to become initialized before
 * allowing the base config object become ready. 
 *  
 */
static fbe_lifecycle_const_base_cond_t base_config_establish_edge_states_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_establish_edge_states_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_ESTABLISH_EDGE_STATES,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_BASE_CONFIG_LIFECYCLE_COND_VALIDATE_DOWNSTREAM_CAPACITY -
 * 
 * This condition is used in the specialize state to validate the downstream capacity.
 *  
 */
static fbe_lifecycle_const_base_cond_t base_config_validate_downstream_capacity_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_validate_downstream_capacity_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_VALIDATE_DOWNSTREAM_CAPACITY,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* base_config_traffic_load_change_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_TRAFFIC_LOAD_CHANGE condition. 
 * we use this condition to indicate the load on the edge has change for better or worse
 * we will propagate this change to the edges above us (propage the worst one)          
 */
static fbe_lifecycle_const_base_cond_t base_config_traffic_load_change_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_traffic_load_change_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_TRAFFIC_LOAD_CHANGE,
        base_config_traffic_load_change_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,             /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,             /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/*
the FBE_BASE_CONFIG_LIFECYCLE_COND_CHECK_NON_PAGED_METADATA_VERSION condition 
is to check if current non-paged metadata is an old version and upgrade it to 
the new version. condition should be only run in ready state.
added by Jingcheng Zhang Apr 27, 2012
*/
static fbe_lifecycle_const_base_timer_cond_t base_config_check_nonpaged_version_cond = {
   {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "base_config_check_nonpaged_metadata_version_cond",
            FBE_BASE_CONFIG_LIFECYCLE_COND_CHECK_NON_PAGED_METADATA_VERSION,
            base_config_check_nonpaged_metadata_version_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,           /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    60000 /* fires every 10 minutes, because version upgrade is very infrequent event */
};


/* base_config_start_hibernation_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_START_HIBERNATION condition. 
 * do what we do when we start hibernating     
 */
static fbe_lifecycle_const_base_cond_t base_config_start_hibernation_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_start_hibernation_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_START_HIBERNATION,
        base_config_start_hibernation_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,             /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,             /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};


/* base_config_dequeue_pending_io_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_DEQUEUE_PENDING_IO condition. 
 * dequeue a pending IO from the block transport server queue    
 */
static fbe_lifecycle_const_base_cond_t base_config_dequeue_pending_io_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_dequeue_pending_io_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_DEQUEUE_PENDING_IO,
        base_config_dequeue_pending_io_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_ACTIVATE,             /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,             /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};


/* base_config_wak_up_hibernating_servers_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_WAKE_UP_HIBERNATING_SERVERS condition. 
 * wake up our servers if they were sleeping  
 */
static fbe_lifecycle_const_base_cond_t base_config_wake_up_hibernating_servers_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_wake_up_hibernating_servers_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_WAKE_UP_HIBERNATING_SERVERS,
        base_config_wake_up_hibernating_servers_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,             /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,             /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};


/* base_config_enable_power_saving_on_servers_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_ENABLE_POWER_SAVING_ON_SERVERS condition. 
 * wake up our servers if they were sleeping  
 */
static fbe_lifecycle_const_base_cond_t base_config_enable_power_saving_on_servers_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_enable_power_saving_on_servers_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_ENABLE_POWER_SAVING_ON_SERVERS,
        base_config_enable_power_saving_on_servers_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,             /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,             /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_BASE_CONFIG_LIFECYCLE_COND_CHECK_POWER_SAVE_DISABLED condition */
static fbe_lifecycle_const_base_timer_cond_t base_config_check_power_save_disabled_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "base_config_check_power_save_disabled_cond",
            FBE_BASE_CONFIG_LIFECYCLE_COND_CHECK_POWER_SAVE_DISABLED,
            base_config_check_power_save_disabled_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,           /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,       /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    1000 /* fires every 10 seconds */
};

static fbe_lifecycle_const_base_cond_t base_config_reset_hibernation_timer_cond =
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_reset_hibernation_timer_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_RESET_HIBERNATION_TIMER,
        base_config_reset_hibernation_timer_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_READY,              /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,            /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,            /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY    */
};


static fbe_lifecycle_const_base_timer_cond_t base_config_hibernation_sniff_wakeup_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "base_config_hibernation_sniff_wakeup_cond",
            FBE_BASE_CONFIG_LIFECYCLE_COND_HIBERNATION_SNIFF_WAKEUP_COND,
            FBE_LIFECYCLE_NULL_FUNC),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,           /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,       /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    6000 /* fires every 1 minue */
};


/* base_config_wait_for_configuration_commit_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_WAIT_FOR_CONFIG_COMMIT condition.
 * The purpose of this condition is to wait for the configuration commit
 * from the configuration service. Base config object will not perform
 * any operation until configuration commits configuration data and sends
 * commit configuration usurpur command to object.
 */
static fbe_lifecycle_const_base_cond_t base_config_wait_for_configuration_commit_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_wait_for_configuration_commit_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_WAIT_FOR_CONFIG_COMMIT,
        fbe_base_config_wait_for_configuration_commit_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_config_set_not_saving_power_cond =
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_set_not_saving_power_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_SET_NOT_SAVING_POWER,
        base_config_set_not_saving_power_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_READY,               /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY    */
};

static fbe_lifecycle_const_base_cond_t base_config_wait_for_server_wakeup_cond =
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_wait_for_server_wakeup_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_WAIT_FOR_SERVERS_WAKEUP,
        base_config_wait_for_server_wakeup_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_READY,               /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY    */
};

static fbe_lifecycle_const_base_cond_t base_config_update_metadata_memory_cond =
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_update_metadata_memory_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY,
        base_config_update_metadata_memory_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_READY,              /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY    */
};

static fbe_lifecycle_const_base_timer_cond_t base_config_update_last_io_time_cond = {
    {
        FBE_LIFECYCLE_DEF_CONST_BASE_TIMER_COND(
            "base_config_update_last_io_time_cond",
            FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_LAST_IO_TIME,
            base_config_update_last_io_time_cond_function),
        FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
            FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
            FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
            FBE_LIFECYCLE_STATE_READY,           /* READY */
            FBE_LIFECYCLE_STATE_HIBERNATE,       /* HIBERNATE */
            FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
            FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
            FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
    },
    FBE_LIFECYCLE_CANARY_CONST_BASE_TIMER_COND,
    1000 /* fires every 10 seconds */
};

static fbe_lifecycle_const_base_cond_t base_config_edge_change_during_hibernation_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_edge_change_during_hibernation_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_EDGE_CHANGE_DURING_HIBERNATION,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_config_persist_non_paged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_persist_non_paged_metadata_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_PERSIST_NON_PAGED_METADATA,
        fbe_base_config_persist_non_paged_metadata_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,            /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* base config_quiesce_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE 
 * The purpose of this condition is to quiesce all I/Os.
 */
static fbe_lifecycle_const_base_cond_t base_config_quiesce_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_quiesce_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* base config_unquiesce_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE 
 * The purpose of this condition is to unquiesce all I/Os.
 */
static fbe_lifecycle_const_base_cond_t base_config_unquiesce_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_unquiesce_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_BASE_CONFIG_LIFECYCLE_COND_PASSIVE_REQUEST_JOIN
 *  
 * This is the condition where we coordinate the passive side joining. 
 * Only the passive side will actually do anything in this condition. 
 * The active side will pass through but automatically clear this condition, since 
 * it automatically has permission to join. 
 */
static fbe_lifecycle_const_base_cond_t base_config_passive_request_join_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_passive_request_join_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_PASSIVE_REQUEST_JOIN,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_BASE_CONFIG_LIFECYCLE_COND_ACTIVE_ALLOW_JOIN
 *  
 * This is the condition where the active side will perform the necessary
 * syncing to allow the passive side to join. 
 */
static fbe_lifecycle_const_base_cond_t base_config_active_allow_join_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_active_allow_join_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_ACTIVE_ALLOW_JOIN,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,       /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,            /* READY */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,          /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,             /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)          /* DESTROY */
};



/* FBE_BASE_CONFIG_LIFECYCLE_COND_BACKGROUND_MONITOR_OPERATION condition.
 * This virtual condition is a common background monitor operation condition
 * to the base config and will be derived by the class which is inherited 
 * from the base config class. The purpose of this condition is that it
 * will remain set all the time and does work only when any background
 * operation needs to run.
 */
static fbe_lifecycle_const_base_cond_t base_config_background_monitor_operation_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_background_monitor_operation_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_BACKGROUND_MONITOR_OPERATION,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_BASE_CONFIG_LIFECYCLE_COND_CLUSTERED_ACTIVATE condition.
 * This condition puts the object in the ACTIVATE state on both SPs.  
 */
static fbe_lifecycle_const_base_cond_t base_config_clustered_activate_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_clustered_activate_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_CLUSTERED_ACTIVATE,
        base_config_clustered_activate_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,             /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

/* FBE_BASE_CONFIG_LIFECYCLE_COND_JOIN_SYNC condition.
 * This condition sync the object both SPs to the JOINED state.  
 */
static fbe_lifecycle_const_base_cond_t base_config_join_sync_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_join_sync_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_JOIN_SYNC,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_BASE_CONFIG_LIFECYCLE_COND_CLEAR_NP condition.
 * This condition clears the Non paged metadata. Currently
 * this condition is used by the PVD only for reinit of system PVD's.  
 */
static fbe_lifecycle_const_base_cond_t base_config_clear_np_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_clear_np_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_CLEAR_NP,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* base_config_set_drive_fault_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_SET_DRIVE_FAULT condition.
 * This condition sets the DRIVE_FAULT attribute. Currently
 * this condition is used by the PVD only.  
 */
static fbe_lifecycle_const_base_cond_t base_config_set_drive_fault_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_set_drive_fault_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_SET_DRIVE_FAULT,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_FAIL,               /* ACTIVATE */
        FBE_LIFECYCLE_STATE_FAIL,               /* READY */
        FBE_LIFECYCLE_STATE_FAIL,               /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* base_config_clear_drive_fault_cond
 * FBE_BASE_CONFIG_LIFECYCLE_COND_CLEAR_DRIVE_FAULT condition.
 * This condition clears the DRIVE_FAULT attribute. Currently
 * this condition is used by the PVD only.  
 */
static fbe_lifecycle_const_base_cond_t base_config_clear_drive_fault_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_clear_drive_fault_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_CLEAR_DRIVE_FAULT,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_BASE_CONFIG_LIFECYCLE_COND_PEER_DEATH_RELEASE_SL condition.
 * This condition is called when the peer has died. This releases
 * all the stripe lock held by the peer
 */
static fbe_lifecycle_const_base_cond_t base_config_peer_death_release_sl_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_peer_death_release_sl_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_PEER_DEATH_RELEASE_SL,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_BASE_CONFIG_LIFECYCLE_COND_REINIT_NP_FROM_PG condition.
 * This condition re-initialize Non paged metadata. Currently
 * this condition is used by the PVD only for rerecreating of system PVD's.  
 */
static fbe_lifecycle_const_base_cond_t base_config_reinit_np_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_reinit_np_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_REINIT_NP,
        FBE_LIFECYCLE_NULL_FUNC),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_BASE_CONFIG_LIFECYCLE_COND_STRIPE_LOCK_START -
 *
 * The purpose of this condition is to start stripe locking.
 * The condition function is implemented in the leaf class. 
 */
static fbe_lifecycle_const_base_cond_t base_config_stripe_lock_start_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_stripe_lock_start_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_STRIPE_LOCK_START,
        NULL),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};
/* FBE_BASE_CONFIG_LIFECYCLE_COND_ENCRYPTION -
 *
 * Handle encryption related details such as removing or pushing keys.
 */
static fbe_lifecycle_const_base_cond_t base_config_encryption_cond = 
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_config_encryption_cond",
        FBE_BASE_CONFIG_LIFECYCLE_COND_ENCRYPTION,
        NULL),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */ 
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */       
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */         
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* base_config_lifecycle_base_cond_array
 * This is our static list of base conditions.
 */
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(base_config)[] = {
    &base_config_downstream_health_broken_cond,
    &base_config_downstream_health_disabled_cond,
    &base_config_detach_block_edges_cond,
    &base_config_metadata_memory_init_cond,
    &base_config_nonpaged_metadata_init_cond,
    &base_config_metadata_element_init_cond,   
    &base_config_zero_consumed_area_cond,
    &base_config_write_default_paged_metadata_cond,
    &base_config_write_default_nonpaged_metadata_cond,
    &base_config_persist_default_nonpaged_metadata_cond,
    &base_config_metadata_verify_cond,
    (fbe_lifecycle_const_base_cond_t *)&base_config_check_nonpaged_version_cond,
    &base_config_negotiate_block_size_cond,
    &base_config_unregister_metadata_element_cond,
    &base_config_traffic_load_change_cond,
    &base_config_establish_edge_states_cond,
    &base_config_validate_downstream_capacity_cond,
    &base_config_start_hibernation_cond,
    &base_config_dequeue_pending_io_cond,
    &base_config_wake_up_hibernating_servers_cond,
    &base_config_enable_power_saving_on_servers_cond,
    (fbe_lifecycle_const_base_cond_t *)&base_config_check_power_save_disabled_cond,
    &base_config_reset_hibernation_timer_cond,
    (fbe_lifecycle_const_base_cond_t*)&base_config_hibernation_sniff_wakeup_cond,
    &base_config_wait_for_configuration_commit_cond,
    &base_config_set_not_saving_power_cond,
    &base_config_wait_for_server_wakeup_cond,
    &base_config_update_metadata_memory_cond,
    (fbe_lifecycle_const_base_cond_t*)&base_config_update_last_io_time_cond,
    &base_config_edge_change_during_hibernation_cond,
    &base_config_persist_non_paged_metadata_cond,
    &base_config_downstream_health_not_optimal_cond,
    &base_config_quiesce_cond,
    &base_config_unquiesce_cond,
    &base_config_passive_request_join_cond,
    &base_config_active_allow_join_cond,
    &base_config_background_monitor_operation_cond,
    &base_config_clustered_activate_cond,
    &base_config_join_sync_cond,
    &base_config_clear_np_cond,
    &base_config_set_drive_fault_cond,
    &base_config_clear_drive_fault_cond,
    &base_config_peer_death_release_sl_cond,
    &base_config_reinit_np_cond,
    &base_config_stripe_lock_start_cond,
    &base_config_encryption_cond,
};


/* base_config_lifecycle_base_conditions 
 *  This is the list of base default base conditions for the .
 *  base config object                                        .
 */
FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(base_config);


/*--- constant rotaries ----------------------------------------------------------------*/
static fbe_lifecycle_const_rotary_cond_t base_config_specialize_rotary[] = {
    /* Derived conditions */

    /* Base conditions */
    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_wait_for_configuration_commit_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_clear_np_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_metadata_memory_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_nonpaged_metadata_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), /* nonpaged metadata init. */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_metadata_element_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), /* metadata element init. */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_reinit_np_cond,FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /* Stripe lock start needs to be before update metadata memory, since update metadata memory
     * will cause the peer to start sending us stripe lock requests.  */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_stripe_lock_start_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_update_metadata_memory_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_downstream_health_not_optimal_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_negotiate_block_size_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_establish_edge_states_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_validate_downstream_capacity_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_zero_consumed_area_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL), /* zero out the area on drive which is consumed by the object. */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_write_default_nonpaged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL), /* Set the default non-paged if applicable. */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_write_default_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL), /* Set all defaults for the signature. */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_persist_default_nonpaged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_metadata_verify_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};


static fbe_lifecycle_const_rotary_cond_t base_config_activate_rotary[] = {
    /* Derived conditions */
    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_update_metadata_memory_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_encryption_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_persist_non_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_downstream_health_disabled_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_set_not_saving_power_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_wake_up_hibernating_servers_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_wait_for_server_wakeup_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_clustered_activate_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};


static fbe_lifecycle_const_rotary_cond_t base_config_fail_rotary[] = {
    /* Derived conditions */

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_update_metadata_memory_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_encryption_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_persist_non_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_clear_np_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_set_drive_fault_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_clear_drive_fault_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_peer_death_release_sl_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_downstream_health_broken_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t base_config_destroy_rotary[] = {
    /* Derived conditions */

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_update_metadata_memory_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_encryption_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_detach_block_edges_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_unregister_metadata_element_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t base_config_ready_rotary[] = {

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_update_metadata_memory_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_encryption_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_join_sync_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_persist_non_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_dequeue_pending_io_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_traffic_load_change_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_update_last_io_time_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_enable_power_saving_on_servers_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_check_nonpaged_version_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_check_nonpaged_version_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    
};

static fbe_lifecycle_const_rotary_cond_t base_config_hibernate_rotary[] = {

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_update_metadata_memory_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_persist_non_paged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_start_hibernation_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_reset_hibernation_timer_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_check_power_save_disabled_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_config_hibernation_sniff_wakeup_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    
};


static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(base_config)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, base_config_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, base_config_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_FAIL, base_config_fail_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, base_config_destroy_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, base_config_ready_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_HIBERNATE, base_config_hibernate_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(base_config);

/*--- global base_config lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_config) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        base_config,
        FBE_CLASS_ID_BASE_CONFIG,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_object))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/

/*--- Condition Functions --------------------------------------------------------------*/

/*!**************************************************************
 * base_config_detach_block_edges_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function where we are trying to
 *  disconnect our edge from the physical drive.
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Always FBE_LIFECYCLE_STATUS_PENDING.
 *
 * @author
 *  05/21/2009 - Created. RPF
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
base_config_detach_block_edges_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_base_config_t * base_config_p = (fbe_base_config_t *)object_p;
    fbe_object_id_t object_id;
    bool is_event_in_fly;

    fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We need to wait the key to be removed first */
    status = fbe_base_config_encryption_check_key_state(base_config_p, packet_p);
    if (status == FBE_STATUS_PENDING)
    {
        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    if (status != FBE_STATUS_CONTINUE)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s waiting encryption keys to be freed\n",
                              __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_base_config_detach_edges(base_config_p, packet_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s edge detach failed, status: 0x%X",
                        __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_object_get_object_id(object_p, &object_id);

    /*  wait for outstanding events to flush which are on the way for object before
     *  we clear the condition
     */
    fbe_event_service_client_is_event_in_fly(object_id, &is_event_in_fly);
    if(is_event_in_fly)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s outstanding event in fly\n",
                        __FUNCTION__);          
            return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Clear the current condition only if detach edge gets passed
     * successfully.
     */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_config_p);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Return pending to the lifecycle.
     */
    return FBE_LIFECYCLE_STATUS_DONE;
}
/*****************************************************
 * end base_config_detach_block_edges_cond_function()
 *****************************************************/

/*!**************************************************************
 * fbe_base_config_pending_func()
 ****************************************************************
 * @brief
 *   This gets called when we are going to transition from one
 *   lifecycle state to another. The purpose here is to drain
 *   all I/O before we transition state.
 *  
 * @param base_config - Object that is pending.
 * @param packet - The monitor paacket.
 * 
 * @return fbe_lifecycle_status_t - We return the status
 *         from fbe_block_transport_server_pending() since
 *         that is the function which deals with determining
 *         when I/Os are drained.
 *
 * @author
 *  05/28/2009 - Created. RPF
 *
 ****************************************************************/

static fbe_lifecycle_status_t 
fbe_base_config_pending_func(fbe_base_object_t *base_object_p, fbe_packet_t * packet)
{
    fbe_lifecycle_status_t lifecycle_status;
    fbe_base_config_t * base_config_p = NULL;
    fbe_lifecycle_status_t event_lifecycle_status;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_status_t            status;


    base_config_p = (fbe_base_config_t *)base_object_p;

    fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    /* Terminator queue will be empty. Well ... eventually */
    base_config_p->resource_priority = 0;

    /* We simply call to the block transport server to handle pending. */
    lifecycle_status = fbe_block_transport_server_pending(&base_config_p->block_transport_server,
                                                          &fbe_base_config_lifecycle_const,
                                                          (fbe_base_object_t *) base_config_p);

    /* get the lifecycly state */
    status = fbe_lifecycle_get_state(&fbe_base_config_lifecycle_const, base_object_p, &lifecycle_state);
    if(status != FBE_STATUS_OK){
        /* KvTrace("%s Critical error fbe_lifecycle_get_state failed, status = %X \n", __FUNCTION__, status); */
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s Critical error fbe_lifecycle_get_state failed, status = %X", __FUNCTION__, status);
        /* We have no error code to return */
    }

    if(lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_DESTROY)
    {
        /* wait for any pending events befor destroy take place */ 
        event_lifecycle_status = fbe_block_transport_drain_event_queue(&base_config_p->block_transport_server);
        if(event_lifecycle_status == FBE_LIFECYCLE_STATUS_PENDING)
        {
            /* set the status based on priority level */
            if(lifecycle_status != FBE_LIFECYCLE_STATUS_RESCHEDULE)
            {
                lifecycle_status =FBE_LIFECYCLE_STATUS_PENDING;
            }
        }
    } 

    return lifecycle_status;
}
/**************************************
 * end fbe_base_config_pending_func()
 **************************************/

/*!**************************************************************
 * base_config_unregister_metadata_element_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function unregisters from the metadata service
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Always FBE_LIFECYCLE_STATUS_PENDING.
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
base_config_unregister_metadata_element_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_base_config_t *                 base_config_p = (fbe_base_config_t *)object_p;
    fbe_payload_ex_t *                  sep_payload = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation = NULL;
    fbe_lifecycle_state_t               peer_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_status_t                        status = FBE_STATUS_OK;


    fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* If is a permanent destroy wait for Peer to be in destroy. Also make sure peer read
     * our metadata while we were in destroy state.
     */
    if(fbe_base_config_is_permanent_destroy(base_config_p) &&
       fbe_base_config_is_peer_present(base_config_p))
    {
        fbe_base_config_metadata_get_peer_lifecycle_state(base_config_p, &peer_state);

        if (peer_state != FBE_LIFECYCLE_STATE_DESTROY)
        {    
            fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                                  FBE_TRACE_LEVEL_DEBUG_LOW,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s peer life: %d wait for destroy.\n", 
                                  __FUNCTION__, peer_state);

            status = fbe_lifecycle_reschedule((fbe_lifecycle_const_t*)&FBE_LIFECYCLE_CONST_DATA(base_config),
                                             (fbe_base_object_t *) base_config_p, (fbe_lifecycle_timer_msec_t) 50);

            return FBE_LIFECYCLE_STATUS_DONE;
        }  
    }
    /* wait until all stripe lock request processed */
    status = fbe_metadata_element_outstanding_stripe_lock_request(&base_config_p->metadata_element);
    if (status == FBE_STATUS_PENDING)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s wait for outstanding stripe lock request\n", 
                                  __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Disable CMI.
     */
    fbe_metadata_element_disable_cmi(&base_config_p->metadata_element);   
    /* wait until after all CMI messages are processed */
    if (base_config_p->metadata_element.attributes & FBE_METADATA_ELEMENT_ATTRIBUTES_CMI_MSG_PROCESSING)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s wait for CMI messages drain\n", 
                                  __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    sep_payload = fbe_transport_get_payload_ex(packet_p);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
 
    fbe_payload_metadata_build_unregister_element(metadata_operation, 
                                                  &base_config_p->metadata_element);

    fbe_transport_set_completion_function(packet_p, base_config_unregister_metadata_element_cond_function_completion, base_config_p);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    fbe_metadata_operation_entry(packet_p);
    
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t base_config_unregister_metadata_element_cond_function_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{

    fbe_status_t                        status;
    fbe_base_config_t *                 base_config_p = NULL;
    fbe_payload_ex_t *                 payload = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation = NULL;
    fbe_payload_metadata_status_t       metadata_status = FBE_PAYLOAD_METADATA_STATUS_FAILURE;
    
    base_config_p = (fbe_base_config_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    metadata_operation = fbe_payload_ex_get_metadata_operation(payload);
    
    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_metadata_get_status(metadata_operation, &metadata_status);

    if ((status == FBE_STATUS_OK) && (metadata_status == FBE_PAYLOAD_METADATA_STATUS_OK))
    {
       /* Clear the current condition*/
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_config_p);
        if (status != FBE_STATUS_OK) {
             fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s can't clear condition, packet failed\n", __FUNCTION__);
        }
    }

    fbe_payload_ex_release_metadata_operation(payload, metadata_operation);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * base_config_traffic_load_change_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function propagate the worst traffic to the upper edges
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
base_config_traffic_load_change_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_base_config_t *         base_config_p = (fbe_base_config_t *)object_p;
    fbe_traffic_priority_t      current_load = FBE_TRAFFIC_PRIORITY_INVALID;
    fbe_traffic_priority_t      highest_load = FBE_TRAFFIC_PRIORITY_INVALID;
    fbe_status_t                status = FBE_STATUS_OK;

    /*what's the highest load on our downstream edges?*/
    highest_load = fbe_base_config_get_downstream_edge_traffic_priority(base_config_p);

    /*what do we advertise to our clients ?*/
    fbe_block_transport_server_get_traffic_priority(&base_config_p->block_transport_server, &current_load);

    /*if we advertise a lower traffic we need to change it*/
    if (highest_load != current_load) {
        status = fbe_block_transport_server_set_traffic_priority(&base_config_p->block_transport_server, highest_load);
    }

    /*and if everything worked nice, let's clear the condition, otherwise leave it here for next cycle*/
    if (status == FBE_STATUS_OK) {
         status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_config_p);
         if (status != FBE_STATUS_OK){
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%X\n",
                                  __FUNCTION__, status);
            
         }
        
    }

    return FBE_LIFECYCLE_STATUS_DONE;

    

}

/*!**************************************************************
 * base_config_check_power_save_cond_function()
 ****************************************************************
 * @brief
 *  check if we need to start saving power based on the time passed since
 *  the last IO
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE
 *
 ****************************************************************/
fbe_lifecycle_status_t base_config_check_power_save_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_base_config_t *         base_config_p = (fbe_base_config_t *)object_p;
    fbe_u32_t                   clients = 0;
    fbe_bool_t                  clients_hibernating = FBE_FALSE;
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t                  need_to_power_save = FBE_FALSE;
    fbe_object_id_t             my_object_id;
    fbe_class_id_t              obj_class_id;
    fbe_path_attr_t             upstream_path_attr = 0;

    fbe_base_object_get_object_id(object_p, &my_object_id);
    obj_class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *)base_config_p));  

    // process the PVD change under VD.                                                                                                  
    if (obj_class_id == FBE_CLASS_ID_VIRTUAL_DRIVE) {  
        fbe_block_transport_server_get_number_of_clients(&base_config_p->block_transport_server, &clients);
        if (clients != 0) {                                                                                          
            fbe_block_transport_server_get_path_attr(&(base_config_p->block_transport_server),                                                       
                                                     0,                                                                                              
                                                     &upstream_path_attr);                                                                           
                                                                                                                                                 
            // only if we(VD) are disabled and the edge above us is hibernating, send usurper to server(PVD)                                   
            if (!base_config_is_object_power_saving_enabled(base_config_p) && (upstream_path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_HIBERNATING))
            {                                                                                                                                        
                status = base_config_send_spindown_qualified_usurper_to_servers(base_config_p, packet_p);                                            
                // if PVD is spin down qualified, re-enable power saving on VD.                                                             
                if (status == FBE_STATUS_OK) { 
                    fbe_base_object_trace(object_p,     
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                          "%s:Enable power saving on vd.\n",
                                          __FUNCTION__);                                                                       
                    //let's enable power saving on this object                                                                                  
                    fbe_base_config_enable_object_power_save(base_config_p);                                                                          
                }                                                                                                                                    
            } 
        }
    } 

    /*PSL objects never save power*/
    if (fbe_private_space_layout_object_id_is_system_object(my_object_id)) {
         return FBE_LIFECYCLE_STATUS_DONE;/*no need to power save*/
    }

    /*see if it's time to power save*/
    status = fbe_base_config_check_if_need_to_power_save(base_config_p, &need_to_power_save);
    if (status != FBE_STATUS_OK) {
         fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s failed to check if need to power save\n",__FUNCTION__);
         return FBE_LIFECYCLE_STATUS_DONE;
    }

    if (!need_to_power_save) {
        return FBE_LIFECYCLE_STATUS_DONE;/*not there yet, we will try again later*/
    }

    if (obj_class_id != FBE_CLASS_ID_PROVISION_DRIVE) {
        status = base_config_send_spindown_qualified_usurper_to_servers(base_config_p, packet_p);
        if (status == FBE_STATUS_INSUFFICIENT_RESOURCES) {
            /*let's also disbale power saving on this object*/
            fbe_base_config_disable_object_power_save(base_config_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }else if (status == FBE_STATUS_BUSY){
            /*some edges were not able to respound, so let's try again in the same cycle*/
            return FBE_LIFECYCLE_STATUS_DONE;
        }else if (status != FBE_STATUS_OK){
            fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't send usurper to servers.\n",__FUNCTION__);
            return FBE_LIFECYCLE_STATUS_DONE;/*we will need to try agin so we don't clear the condition*/
        }
    }   
    
    /*ok, we had enough time w/o IO, but can we really go to hibernation ?,here are the rules:
    1) No edge above us, OK
    2) ALL edges above us are marked SLUMBER - OK*/
    fbe_block_transport_server_get_number_of_clients(&base_config_p->block_transport_server, &clients);
    if (clients == 0) {
        /*we are good to go, there is no one above us*/
        if (!fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY))
        {
            fbe_base_object_trace(object_p,     
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                  "%s: ready to go hibernating.\n",
                                  __FUNCTION__);                                                                       
            /*mark local side request to go hibernation */
            fbe_base_config_set_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY);
        }

        /* only the active side goes into hibernation here, and passive side will follow
         * after active goes into hibernate state 
         */
        if (!fbe_base_config_is_peer_present(base_config_p) ||
            (fbe_base_config_is_peer_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY) &&
             fbe_base_config_is_active(base_config_p)))
        {
            status = fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                                            object_p, 
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_HIBERNATE);

            if (status != FBE_STATUS_OK) {
                fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s can't set object to HIBERNATE\n",__FUNCTION__);
            }
        }
        
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /*we have some edges above us. We need to make sure they are all hibernating*/
    clients_hibernating = fbe_block_transport_server_all_clients_hibernating(&base_config_p->block_transport_server);
    if (clients_hibernating) {
        if (!fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY))
        {
            fbe_base_object_trace(object_p,     
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                  "%s: ready to go hibernating.\n",
                                  __FUNCTION__);                                                                       
            /*mark local side request to go hibernation */
            fbe_base_config_set_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY);
        }

        /* only the active side goes into hibernation here, and passive side will follow
         * after active goes into hibernate state 
         */
        if (!fbe_base_config_is_peer_present(base_config_p) ||
            (fbe_base_config_is_peer_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY) &&
             fbe_base_config_is_active(base_config_p)))
        {
            /*we are good to go, everyone above us is hibernating*/
            status = fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                                            object_p, 
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_HIBERNATE);

            if (status != FBE_STATUS_OK) {
                fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s can't set object to HIBERNATE\n",__FUNCTION__);
            }
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /*not all edges are hibernating, let's try again later*/
    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t base_config_start_hibernation_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_config_t *     base_config_p = (fbe_base_config_t *)base_object;

    /* Lock base config for updating the state */
    fbe_base_config_lock(base_config_p);

    /* This information will show up only on our side and not on the peer since it's not relevant for the peer.
     * When we are here, Hibernation already started. Hence, we need to set the FBE_POWER_SAVE_STATE_SAVING_POWER 
     * state instead of FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE.  Let's mark the metadata portion to say we now 
     * save power(if we are the passive side it will let the active side know that it's ready to save power as well 
     * since it has to wait for the passive to go down first.  The reason is that saving power will spin down or 
     * power down the drives and if the passive side does not go to hibernate first, any IO that will come or 
     * any other thing will see the spinning down or powering down drives as a topology error 
     */
    fbe_base_config_metadata_set_power_save_state(base_config_p, FBE_POWER_SAVE_STATE_SAVING_POWER);

    /* UnLock base config after updating the state */
    fbe_base_config_unlock(base_config_p);

    /*send down a ususrper command to our servers to indicate we are now hibernating*/
    status  = base_config_send_hibernation_usurper_to_servers(base_config_p, packet);

    if (status == FBE_STATUS_INSUFFICIENT_RESOURCES) {

        /* Lock base config for updating the state */
        fbe_base_config_lock(base_config_p);

        /*no point on hibernating, non of our severs support power save just get back to ready*/
        fbe_base_config_metadata_set_power_save_state(base_config_p, FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE);

        /* Unlock base config after updating the state */
        fbe_base_config_unlock(base_config_p);

        /*let's also disbale power saving on this object*/
        fbe_base_config_disable_object_power_save(base_config_p);

         /*and get back to ready*/
        fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                                 base_object, 
                                 FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);

        return FBE_LIFECYCLE_STATUS_DONE;

    }else if (status == FBE_STATUS_BUSY){

        /*some edges were not able to set hibernation, so let's try again in the same cycle*/
        return FBE_LIFECYCLE_STATUS_DONE;
    }else if (status != FBE_STATUS_OK){

        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't send hiebrnation usurper to servers, status: 0x%X\n",
                              __FUNCTION__, status);

        return FBE_LIFECYCLE_STATUS_DONE;/*we will need to try agin so we don't clear the condition*/
    }

    /*set the condition so when we wake up, we will wake up our servers*/
    fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                             base_object, 
                             FBE_BASE_CONFIG_LIFECYCLE_COND_WAKE_UP_HIBERNATING_SERVERS);

     fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "Object is now Hibernating\n");

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    /*and set the condition that will update the metadata memory area for the peer*/
    status = fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                                    base_object, 
                                    FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s can't set condition\n",__FUNCTION__);
    }

    return FBE_LIFECYCLE_STATUS_DONE;

}

static fbe_lifecycle_status_t base_config_dequeue_pending_io_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_config_t *     base_config_p = (fbe_base_config_t *)base_object;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Sending IO that was queued when we were not READY\n",
                              __FUNCTION__);

    /*need to send all IOs that were queud for some reason(e.g. we were hibernating and we got IO)*/
    fbe_block_transport_server_process_io_from_queue(&base_config_p->block_transport_server);

    return FBE_LIFECYCLE_STATUS_DONE;
}


static fbe_lifecycle_status_t base_config_wake_up_hibernating_servers_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_config_t *     base_config_p = (fbe_base_config_t *)base_object;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_time_t              current_time = 0;
    fbe_scheduler_hook_status_t         hook_status;

    /*reset the last IO time so we won't go back to sleep too early*/
    block_transport_server_set_attributes(&base_config_p->block_transport_server,
                                          FBE_BLOCK_TRANSPORT_FLAGS_IO_IN_PROGRESS);

    current_time = fbe_get_time();
    fbe_block_transport_server_set_last_io_time(&base_config_p->block_transport_server, current_time);

    /*clear the path state saying we save power*/
    status = fbe_block_transport_server_clear_path_attr_all_servers(&base_config_p->block_transport_server, FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_READY_TO_HIBERNATE);
    if (status != FBE_STATUS_OK) {
         fbe_base_object_trace(base_object, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s failed to clear path attribute\n",__FUNCTION__);
    }

    /* Lock base config for updating the state */
    fbe_base_config_lock(base_config_p);
    /*this is updated locally only, the peer does not care about that*/
    fbe_base_config_metadata_set_power_save_state(base_config_p, FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE);
    /* Unlock base config after updating the state */
    fbe_base_config_unlock(base_config_p);

    status = fbe_base_config_send_wake_up_usurper_to_servers(base_config_p, packet);
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to wake up servers, status: 0x%X\n",__FUNCTION__, status);
    }

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    fbe_base_object_trace(base_object, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "Object no longer Hibernating\n");
    
    fbe_base_config_check_hook(base_config_p,  SCHEDULER_MONITOR_STATE_BASE_CONFIG_OUT_OF_HIBERNATE,
                                   FBE_BASE_CONFIG_SUBSTATE_OUT_OF_HIBERNATION, 0, &hook_status);


    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Lock base config for updating the state */
    fbe_base_config_lock(base_config_p);
    fbe_base_config_metadata_set_power_save_state(base_config_p, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);
    /* Unlock base config after updating the state */
    fbe_base_config_unlock(base_config_p);

    if (fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY))
    {
        fbe_base_config_clear_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY);
    }

    /*and set the condition that will update the metadata memory area for the peer*/
    status = fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                                    base_object, 
                                    FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);


    /*we also want to set a condition that verified the servers are ready before we get out of activate*/
    fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                            base_object, 
                            FBE_BASE_CONFIG_LIFECYCLE_COND_WAIT_FOR_SERVERS_WAKEUP);

    return FBE_LIFECYCLE_STATUS_DONE;

}

static fbe_lifecycle_status_t base_config_enable_power_saving_on_servers_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_config_t *     base_config_p = (fbe_base_config_t *)base_object;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_base_config_send_power_save_enable_usurper_to_servers(base_config_p, packet);
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to enable some or all servers, status: 0x%X\n",__FUNCTION__, status);
    }

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t base_config_check_power_save_disabled_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_config_t *             base_config_p = (fbe_base_config_t *)base_object;
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_system_power_saving_info_t  system_ps_info;
    fbe_bool_t                      object_ps_enabled = FBE_FALSE;

    /*let's check if the user disabled the system power saving*/
    fbe_base_config_get_system_power_saving_info(&system_ps_info);
    
    /*let's check if someone disabled our own power saving*/
    object_ps_enabled = base_config_is_object_power_saving_enabled(base_config_p);
    
    if (!system_ps_info.enabled || !object_ps_enabled) {

        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "system or object power saving disabled, going out of hibernation\n");

        /* Lock base config for updating the state */
        fbe_base_config_lock(base_config_p);
        /*let's get out of hibernation*/
        fbe_base_config_metadata_set_power_save_state(base_config_p, FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE);
        /* Unlock base config after updating the state */
        fbe_base_config_unlock(base_config_p);

        fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                               base_object, 
                               FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);

    }

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(base_object, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;

}

static fbe_lifecycle_status_t base_config_reset_hibernation_timer_cond_function( fbe_base_object_t* in_object_p,
                                                                                      fbe_packet_t*      in_packet_p  )
{
    fbe_base_config_t *             base_config_p = (fbe_base_config_t *)in_object_p;
    fbe_status_t                    status;

    base_config_p->hibernation_time = fbe_get_time();

    status = fbe_lifecycle_clear_current_cond(in_object_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(in_object_p, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s failed to clear condition %d\n", __FUNCTION__, status);                

    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!****************************************************************************
 * fbe_base_config_wait_for_configuration_commit_function()
 ******************************************************************************
 * @brief
 *  This condition function allows object to wait until configuration gets
 *  commited by the configuration service. It runs as first condition in
 *  specialize state and wait for the configuration commit before it gets
 *  cleared.
 * 
 * @param base_config_p         - Pointer to the base config object.
 * @param packet_p              - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/24/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_lifecycle_status_t
fbe_base_config_wait_for_configuration_commit_function(fbe_base_object_t * base_object_p,
                                                       fbe_packet_t * packet_p)
{
    fbe_base_config_t * base_config_p = (fbe_base_config_t *) base_object_p;
    fbe_bool_t          is_configuration_commited;
    fbe_status_t        status;

    /* verify whether configuration gets commited or not, if we find that
     * configuration is commited then clear this condition.
     */
    is_configuration_commited = fbe_base_config_is_configuration_commited(base_config_p);
    if(is_configuration_commited)
    {
        /* clear the wait for configuration commit condition. */
        status = fbe_lifecycle_clear_current_cond(base_object_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(base_object_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
    }    

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_base_config_wait_for_configuration_commit_function()
 ******************************************************************************/


static fbe_lifecycle_status_t base_config_set_not_saving_power_cond_function( fbe_base_object_t* in_object_p,fbe_packet_t* in_packet_p  )
{
    fbe_base_config_t *     base_config_p = (fbe_base_config_t *)in_object_p;
    fbe_status_t            status;
    
    /* Lock base config for updating the state */
    fbe_base_config_lock(base_config_p);
    /*when we'll go to ready or FAIL this will get updated on the peer as well*/
    fbe_base_config_metadata_set_power_save_state(base_config_p, FBE_POWER_SAVE_STATE_NOT_SAVING_POWER);
    fbe_base_config_metadata_set_last_io_time(base_config_p, 0);
    /* Unlock base config after updating the state */
    fbe_base_config_unlock(base_config_p);

    if (fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY))
    {
        fbe_base_config_clear_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY);
    }
    
    status = fbe_lifecycle_clear_current_cond(in_object_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(in_object_p, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s failed to clear condition %d\n", __FUNCTION__, status);                

    }

    /*and set the condition that will update the metadata memory area for the peer*/
    status = fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                                    in_object_p, 
                                    FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(in_object_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,"%s can't set condition\n",__FUNCTION__);
    }

    return FBE_LIFECYCLE_STATUS_DONE;

}

static fbe_lifecycle_status_t base_config_wait_for_server_wakeup_cond_function( fbe_base_object_t* in_object_p,fbe_packet_t* in_packet_p  )
{
    fbe_base_config_t *             base_config_p = (fbe_base_config_t *)in_object_p;
    fbe_status_t                    status;

    status = fbe_base_config_power_save_verify_servers_not_slumber(base_config_p);
    if (status == FBE_STATUS_BUSY) {
        return FBE_LIFECYCLE_STATUS_DONE;/*some of them are still waking up*/
    }else if (status != FBE_STATUS_OK) {
            fbe_base_object_trace(in_object_p, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s failed to check servers are woken up %d\n", __FUNCTION__, status);    

    }

    status = fbe_lifecycle_clear_current_cond(in_object_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(in_object_p, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s failed to clear condition %d\n", __FUNCTION__, status);                

    }

    return FBE_LIFECYCLE_STATUS_DONE;

}

static fbe_status_t 
fbe_base_config_check_nonpaged_read_persist_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t                        status;
    fbe_bool_t                          is_nonpaged_initialized = FALSE;
    fbe_base_config_t *                 base_config = NULL;
    fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata_p = NULL;
    
    base_config = (fbe_base_config_t*)context;
    
    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s failed to read persist nonpaged %d\n", __FUNCTION__, status);                
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_config_metadata_is_initialized(base_config, &is_nonpaged_initialized);
    if (!is_nonpaged_initialized)
    {
        /* If the read succeeds but nonpaged is still not initialized, we need to clear the cond
         * and let metadata_verify cond to initialize everything.
         */
        fbe_base_object_trace((fbe_base_object_t*)base_config, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s nonpaged not initialized after disk read\n", __FUNCTION__);                
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_config);

        fbe_base_config_get_nonpaged_metadata_ptr(base_config, (void **)&base_config_nonpaged_metadata_p);
        if(base_config_nonpaged_metadata_p != NULL)
        {
            fbe_base_config_nonpaged_metadata_set_np_state(base_config_nonpaged_metadata_p, FBE_NONPAGED_METADATA_UNINITIALIZED);
        }

    }

    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t 
base_config_check_nonpaged_request(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_status_t                        status;
    fbe_lifecycle_state_t               lifecycle_state;    
    fbe_base_config_nonpaged_metadata_t *base_config_nonpaged_metadata_p = NULL;
    fbe_bool_t                          is_nonpaged_initialized = FALSE;
    fbe_bool_t                          is_peer_request;
    fbe_bool_t                          is_peer_alive;
    fbe_bool_t                          is_peer_object_alive;
    fbe_object_id_t                     object_id;
    fbe_object_id_t                     last_system_object_id;
    fbe_bool_t                          is_system_object;
    fbe_bool_t                          is_initial_creation;
    fbe_bool_t                          is_nonpaged_loaded;

    fbe_lifecycle_get_state(&fbe_base_config_lifecycle_const, (fbe_base_object_t *) base_config, &lifecycle_state);

    fbe_base_config_get_nonpaged_metadata_ptr(base_config, (void **)&base_config_nonpaged_metadata_p);
    if(base_config_nonpaged_metadata_p == NULL){ /* The nonpaged pointers are not initialized yet */
        /* Just keep moving */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_config);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    if(lifecycle_state == FBE_LIFECYCLE_STATE_DESTROY){
        return FBE_LIFECYCLE_STATUS_CONTINUE;
    }

    /* If the lifecycle is specialize check the `init non-paged' hook.
     */
    if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
    {
        fbe_scheduler_hook_status_t hook_status;

        fbe_base_config_check_hook(base_config,
                                   SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_UPDATE,
                                   FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_CHECK,
                                   0,
                                   &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE)
        {
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }

    fbe_base_config_metadata_is_initialized(base_config, &is_nonpaged_initialized);

    if (lifecycle_state == FBE_LIFECYCLE_STATE_READY && is_nonpaged_initialized == FBE_FALSE) {
        fbe_base_object_trace((fbe_base_object_t*)base_config,
                                FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s this is bad can't be ready and not initialized", __FUNCTION__);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If the peer_set_request to sync non-paged metadata is TRUE,
     * check for ACTIVE SP and non-page initiated. If so, go and update the
     * non-paged metadata. 
     */ 
    is_peer_request = fbe_base_config_is_peer_clustered_flag_set(base_config, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST);
    is_peer_alive = fbe_cmi_is_peer_alive();
    is_peer_object_alive = fbe_metadata_is_peer_object_alive(&base_config->metadata_element);
    object_id = fbe_base_config_metadata_element_to_object_id(&base_config->metadata_element);
    fbe_database_get_last_system_object_id(&last_system_object_id);
    is_system_object = (object_id <= last_system_object_id) ? FBE_TRUE : FBE_FALSE;
    is_nonpaged_loaded = fbe_database_get_metadata_nonpaged_loaded();
    is_initial_creation = fbe_base_config_is_initial_configuration(base_config);

    /* Handle destroy case */
    if((lifecycle_state == FBE_LIFECYCLE_STATE_DESTROY)  && (!is_peer_object_alive)){
        /* We are down and there is no peer */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_config);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* If the peer is not alive and the non-paged is not initialized trace some 
     * information.
     */
    if (!is_peer_alive && !is_peer_request && !is_nonpaged_initialized)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s init: %d peer alive: %d peer: %d system: %d initial: %d load: %d\n",
                              __FUNCTION__, is_nonpaged_initialized, is_peer_alive, is_peer_object_alive,
                              is_system_object, is_initial_creation, is_nonpaged_loaded);
    }

    /* update the non-paged from peer */
    if (is_peer_alive && is_peer_request && is_nonpaged_initialized) 
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "PEER_SET_REQUEST is SET. Update the non-paged data\n");

        /*  Send the nonpaged metadata update request to metadata service. */
        status = fbe_base_config_metadata_nonpaged_update((fbe_base_config_t *) base_config, packet);

        return FBE_LIFECYCLE_STATUS_PENDING;
    }
    else if ( !is_nonpaged_initialized && !is_peer_alive       && !is_peer_object_alive && 
              !is_initial_creation     && !is_nonpaged_loaded                              )     
    {
        {
            /*hook check*/
            fbe_scheduler_hook_status_t hook_status;
            fbe_base_config_check_hook(base_config,
                                       SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_UPDATE,
                                       FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_LOAD_FROM_DISK,
                                       0,
                                       &hook_status);
            if (hook_status == FBE_SCHED_STATUS_DONE)
            {
                return FBE_LIFECYCLE_STATUS_DONE;
            }
        }
    
        fbe_base_object_trace((fbe_base_object_t*)base_config,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "PEER DEAD: Read non-paged data from disk\n");

        fbe_transport_set_completion_function(packet, fbe_base_config_check_nonpaged_read_persist_completion, base_config);
        /*  Send the nonpaged metadata read request to metadata service. */
        status = fbe_base_config_metadata_nonpaged_read_persist((fbe_base_config_t *) base_config, packet);

        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    return FBE_LIFECYCLE_STATUS_CONTINUE;
}

static fbe_lifecycle_status_t 
base_config_check_passive_request(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    if(fbe_base_config_is_active(base_config)){
        if(fbe_base_config_is_clustered_flag_set(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_REQ)){
            /* We requesting to be passive */
            fbe_metadata_element_set_state(&base_config->metadata_element, FBE_METADATA_ELEMENT_STATE_PASSIVE);
            fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    "%s Set PASSIVE state\n", __FUNCTION__);

            return FBE_LIFECYCLE_STATUS_CONTINUE;
        }

        if(!fbe_base_config_is_peer_clustered_flag_set(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_REQ) &&
            fbe_base_config_is_clustered_flag_set(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_STARTED)){
            fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    "%s Active: Clear PASSIVE REQ\n", __FUNCTION__);

            fbe_base_config_clear_clustered_flag(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_MASK);
        }

    } else { /* We are passive and only allowed to become active once we init nonpaged. */
        if(fbe_base_config_is_peer_clustered_flag_set(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_REQ)){

           if (!fbe_base_config_metadata_element_is_nonpaged_initialized(&base_config->metadata_element)){

               fbe_base_object_trace(  (fbe_base_object_t *)base_config, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                       "%s passive not initialized yet, process PASSIVE_REQ later.\n", __FUNCTION__);
               return FBE_LIFECYCLE_STATUS_CONTINUE;
           }
            /* Peer is already PASSIVE, so we will go active */
            fbe_metadata_element_set_state(&base_config->metadata_element, FBE_METADATA_ELEMENT_STATE_ACTIVE);
            fbe_base_config_set_clustered_flag(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_STARTED);
            fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    "%s Set ACTIVE state\n", __FUNCTION__);

            return FBE_LIFECYCLE_STATUS_CONTINUE;
        }
        
        if(fbe_base_config_is_peer_clustered_flag_set(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_STARTED) &&
            fbe_base_config_is_clustered_flag_set(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_REQ)){
            fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    "%s Passive: Clear PASSIVE REQ\n", __FUNCTION__);

            /* Peer just became ACTIVE */
            fbe_base_config_clear_clustered_flag(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_MASK);
        }       
    }

    return FBE_LIFECYCLE_STATUS_CONTINUE;
}

static fbe_lifecycle_status_t 
base_config_update_metadata_memory_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{    
    fbe_lifecycle_state_t lifecycle_state;
    fbe_lifecycle_state_t peer_lifecycle_state;
    fbe_base_config_metadata_memory_t * base_config_metadata_memory = NULL;
    fbe_base_config_t * base_config = NULL;
    fbe_payload_ex_t *                 sep_payload = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation = NULL;
    fbe_lifecycle_status_t lifecycle_status;

    base_config = (fbe_base_config_t *)base_object;

    fbe_base_object_trace((fbe_base_object_t*)base_config,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    fbe_lifecycle_get_state(&fbe_base_config_lifecycle_const, (fbe_base_object_t *) base_config, &lifecycle_state);

    /* Based on current lifecycle state it updates the metadata memory lifecycle */
    fbe_metadata_element_get_metadata_memory(&base_config->metadata_element, (void **)&base_config_metadata_memory); 

    /* It is possible that an object creation was aborted and it is a not completely initialized
     * object being destroyed.
     */
    if((FBE_LIFECYCLE_STATE_DESTROY == lifecycle_state) && (NULL == base_config_metadata_memory))
    {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_config);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /*  If we are switching from passive to active, make sure we change the element state */
    if (!fbe_metadata_element_is_active(&base_config->metadata_element) && !fbe_cmi_is_peer_alive())
    {
        fbe_metadata_element_set_state(&base_config->metadata_element, FBE_METADATA_ELEMENT_STATE_ACTIVE);
    }

    base_config_metadata_memory->lifecycle_state = lifecycle_state;

    lifecycle_status = base_config_check_nonpaged_request(base_config, packet);
    if(lifecycle_status != FBE_LIFECYCLE_STATUS_CONTINUE){
        return lifecycle_status;
    }

    if(fbe_cmi_is_peer_alive()){ /* Quick fix for AR 542776 Note: We need to carefully evaluate the check for other requests */
        lifecycle_status = base_config_check_passive_request(base_config, packet);
        if(lifecycle_status != FBE_LIFECYCLE_STATUS_CONTINUE){
            return lifecycle_status;
        }
    }

    /* On Passive side we should not send memory updates if peer is in specialize state */
    fbe_base_config_metadata_get_peer_lifecycle_state(base_config, &peer_lifecycle_state);

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
    fbe_payload_metadata_build_memory_update(metadata_operation, &base_config->metadata_element);
    fbe_transport_set_completion_function(packet, base_config_update_metadata_memory_cond_function_completion, base_config);
    fbe_payload_ex_increment_metadata_operation_level(sep_payload);
    fbe_metadata_operation_entry(packet);
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
base_config_update_metadata_memory_cond_function_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{

    fbe_status_t                        status;
    fbe_base_config_t *                 base_config = NULL;
    fbe_payload_ex_t *                 payload = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation = NULL;
    fbe_payload_metadata_status_t       metadata_status;
    fbe_bool_t is_active;
    fbe_bool_t is_nonpaged_initialized;
    fbe_bool_t is_nonpaged_request_set;
    
    base_config = (fbe_base_config_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    metadata_operation = fbe_payload_ex_get_metadata_operation(payload);
    
    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_metadata_get_status(metadata_operation, &metadata_status);

    is_nonpaged_initialized = fbe_base_config_metadata_nonpaged_is_initialized(base_config, &is_nonpaged_initialized);
    is_active = fbe_base_config_is_active(base_config);
    is_nonpaged_request_set = fbe_base_config_is_clustered_flag_set(base_config, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST);

    if(!is_active && is_nonpaged_request_set && !is_nonpaged_initialized){
        fbe_payload_ex_release_metadata_operation(payload, metadata_operation);
        return FBE_STATUS_OK;
    }

   /* Clear the current condition*/
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_config);
    if (status != FBE_STATUS_OK) {
         fbe_base_object_trace((fbe_base_object_t*)base_config,
                      FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s can't clear condition, packet failed\n", __FUNCTION__);
    }


    fbe_payload_ex_release_metadata_operation(payload, metadata_operation);

    return FBE_STATUS_OK;
}


/*if we are the passive side and we run IO, we want to make sure the active side know we can't be spinning down yet
To do so, we ysnch with the peer the last IO time, When the active peer sees it is not 0 it means it can now spin down*/
static fbe_lifecycle_status_t base_config_update_last_io_time_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t                        status;
    fbe_base_config_t *                 base_config = NULL;
    fbe_payload_ex_t *                 sep_payload = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation = NULL;
    fbe_time_t                          last_io_time, last_recorded_io_time = 0;
    fbe_u32_t                           elapsed_seconds = 0;

    base_config = (fbe_base_config_t *)object_p;

    fbe_base_object_trace((fbe_base_object_t*)base_config,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    /*let's get the time now and the time of the last IO*/
    fbe_block_transport_server_get_last_io_time(&base_config->block_transport_server, &last_io_time);
    fbe_base_config_metadata_get_last_io_time(base_config, &last_recorded_io_time);

    elapsed_seconds = fbe_get_elapsed_seconds(last_io_time);
    /*before we do anything, let's update the idle time on our end first*/
    if ((elapsed_seconds < base_config->power_saving_idle_time_in_seconds) ||
        (base_config->block_transport_server.attributes & FBE_BLOCK_TRANSPORT_FLAGS_IO_IN_PROGRESS)) {
    
        /*if there is still IO in progeress we make sure to update the last time we were here,
        This will save us access to the CPU on each IO just ot update the last IO time*/
        if (base_config->block_transport_server.attributes & FBE_BLOCK_TRANSPORT_FLAGS_IO_IN_PROGRESS) {
            block_transport_server_clear_attributes(&base_config->block_transport_server,
                                                    FBE_BLOCK_TRANSPORT_FLAGS_IO_IN_PROGRESS);
            fbe_block_transport_server_set_last_io_time(&base_config->block_transport_server, fbe_get_time());
        }
        
    }

    /*we want to update the other side only if there is a change and not every few seconds.
    Also, if we already updated the last io time (last_recorded_io_time != 0) then we won't do it again */
    if ((elapsed_seconds < base_config->power_saving_idle_time_in_seconds) ||
        (last_recorded_io_time != 0)) {

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_config);
        if (status != FBE_STATUS_OK) {
         fbe_base_object_trace((fbe_base_object_t*)base_config,
                      FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s can't clear condition, packet failed\n", __FUNCTION__);
        }   

        return FBE_LIFECYCLE_STATUS_DONE;/*we can continue execution*/
    }
    fbe_base_config_metadata_set_last_io_time(base_config, last_io_time);
    
    sep_payload = fbe_transport_get_payload_ex(packet_p);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
 
    fbe_payload_metadata_build_memory_update(metadata_operation, &base_config->metadata_element);

    fbe_transport_set_completion_function(packet_p, base_config_update_last_io_time_cond_function_completion, base_config);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    fbe_metadata_operation_entry(packet_p);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
base_config_update_last_io_time_cond_function_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{

    fbe_status_t                        status;
    fbe_base_config_t *                 base_config = NULL;
    fbe_payload_ex_t *                 payload = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation = NULL;
    fbe_payload_metadata_status_t       metadata_status;
    
    base_config = (fbe_base_config_t*)context;

    payload = fbe_transport_get_payload_ex(packet);
    metadata_operation = fbe_payload_ex_get_metadata_operation(payload);
    
    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_metadata_get_status(metadata_operation, &metadata_status);

   /* Clear the current condition*/
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_config);
    if (status != FBE_STATUS_OK) {
         fbe_base_object_trace((fbe_base_object_t*)base_config,
                      FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s can't clear condition, packet failed\n", __FUNCTION__);
    }


    fbe_payload_ex_release_metadata_operation(payload, metadata_operation);

    return FBE_STATUS_OK;
}


fbe_lifecycle_status_t fbe_base_config_persist_non_paged_metadata_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_scheduler_hook_status_t hook_status;

    if (fbe_base_config_is_active((fbe_base_config_t *)object_p)) {
        fbe_base_config_check_hook((fbe_base_config_t *)object_p,
                                   SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_PERSIST,
                                   FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_PERSIST,
                                   0,
                                   &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE)
        {
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        fbe_transport_set_completion_function(packet_p, base_config_persist_non_paged_metadata_cond_function_completion, object_p);
        fbe_base_config_metadata_nonpaged_persist((fbe_base_config_t *)object_p, packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }else{
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

}

static fbe_status_t 
base_config_persist_non_paged_metadata_cond_function_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{

    fbe_base_config_t *                 base_config = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    base_config = (fbe_base_config_t *)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK)){
        /*persistance worked, we are good*/
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) base_config);
       
    }else{
        fbe_base_object_trace((fbe_base_object_t *)base_config,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s pesrist nonpaged metadata condition failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * base_config_clustered_activate_cond_function()
 ****************************************************************
 * @brief
 *  This handler function for clustered activate condition
 *
 * @param base_config_p - object  
 *        packet_p      - Lifecycle packet           
 *
 * @return fbe_status_t
 *
 * @author
 *  01/20/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
base_config_clustered_activate_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_base_config_t *                 base_config_p = NULL;
    fbe_scheduler_hook_status_t			hook_status;
    
    base_config_p = (fbe_base_config_t *)object_p;

    fbe_base_object_trace((fbe_base_object_t*)object_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    fbe_base_config_check_hook(base_config_p,  SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE,
                                   FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_ENTRY, 0, &hook_status);


    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_config_lock(base_config_p);

    /* If we are coming in here for the first time, then the set the request flag
     */
    if ((!fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_REQ)) &&
        (!fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_STARTED)))
    {
        fbe_base_config_set_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_REQ);

        fbe_base_config_unlock(base_config_p);
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Marking FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_REQ\n");


       return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    if (fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_REQ))
    {
        fbe_base_config_unlock(base_config_p);
        return fbe_base_config_clustered_activate_request(base_config_p, packet_p);
    }

    if (fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_STARTED))
    {
        fbe_base_config_unlock(base_config_p);
        return fbe_base_config_clustered_activate_started(base_config_p, packet_p);
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * fbe_base_config_clustered_activate_request()
 ****************************************************************
 * @brief
 *  This function handles the clustered activate request state
 *
 * @param base_config_p - object             
 *
 * @return fbe_status_t
 *
 * @author
 *  01/20/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_base_config_clustered_activate_request(fbe_base_config_t *  base_config_p, fbe_packet_t * packet_p)
{
    fbe_bool_t           b_peer_flag_set = FBE_FALSE;
    fbe_status_t         status;
    fbe_scheduler_hook_status_t        hook_status = FBE_SCHED_STATUS_OK;

    fbe_base_config_lock(base_config_p);

    fbe_base_config_check_hook(base_config_p,
                               SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE,
                               FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_REQUEST,
                               0,
                               &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE) 
    {
        fbe_base_config_unlock(base_config_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    b_peer_flag_set = fbe_base_config_is_peer_clustered_flag_set(base_config_p,
                                                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_STARTED);

    if (b_peer_flag_set || !fbe_base_config_is_peer_present(base_config_p)) 
    {
        fbe_base_config_check_hook(base_config_p,
                               SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE,
                               FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_CLEARED,
                               0,
                               &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) 
        {
            fbe_base_config_unlock(base_config_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        /* The peer has set the started flag, which means it has seen us setting the req flag and
         * acknowledge it. At this point, the Objects on both the SPs are in 
         * activate state. So clear the flags and the condition
         */
        fbe_base_config_clear_clustered_flag(base_config_p, 
                                             (FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_STARTED | 
                                              FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_REQ));

        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_config_p);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        fbe_base_config_unlock(base_config_p);
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Clearing the clustered activate condition\n");
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

    b_peer_flag_set = fbe_base_config_is_peer_clustered_flag_set(base_config_p,
                                                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_REQ);
    if (b_peer_flag_set) 
    {
        fbe_base_config_check_hook(base_config_p,
                               SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE,
                               FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_STARTED,
                               0,
                               &hook_status);
        if (hook_status == FBE_SCHED_STATUS_DONE) 
        {
            fbe_base_config_unlock(base_config_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }

        fbe_base_config_set_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_STARTED);

        /* Both the SPs have set the REQ flag, move on to the next phase
         */
        fbe_base_config_clear_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_REQ);
        fbe_base_config_unlock(base_config_p);
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Marking FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_STARTED\n");
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    } 
    else 
    {
        /* Peer is not yet running the condition, wait for them to start running this condition.
         */
        fbe_base_config_unlock(base_config_p);
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Waiting for Peer to set Req\n", __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
}

/*!**************************************************************
 * fbe_base_config_clustered_activate_started()
 ****************************************************************
 * @brief
 *  This function handles the clustered activate started state
 *
 * @param base_config_p - object             
 *
 * @return fbe_status_t
 *
 * @author
 *  01/20/2012 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_base_config_clustered_activate_started(fbe_base_config_t *  base_config_p, fbe_packet_t * packet_p)
{
    fbe_scheduler_hook_status_t        hook_status = FBE_SCHED_STATUS_OK;
    fbe_bool_t           b_peer_flag_set = FBE_FALSE;
    fbe_status_t status;

    fbe_base_config_lock(base_config_p);

     b_peer_flag_set = fbe_base_config_is_peer_clustered_flag_set(base_config_p,
                                                                 FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_REQ);
    if (!b_peer_flag_set || !fbe_base_config_is_peer_present(base_config_p)) 
    {
        fbe_base_config_check_hook(base_config_p,
                               SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE,
                               FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_CLEARED,
                               0,
                               &hook_status);

        if (hook_status == FBE_SCHED_STATUS_DONE) 
        {
            fbe_base_config_unlock(base_config_p);
            return FBE_LIFECYCLE_STATUS_DONE;
        }
        /* We will be in this case, only when both the SPs have set the REQ flag. Since the flag is now
         * cleared means that peer also knows that we have seen the REQ flag and he has moved on. So 
         * let us do the same.
         */
        fbe_base_config_clear_clustered_flag(base_config_p, 
                                             (FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_STARTED | 
                                              FBE_BASE_CONFIG_CLUSTERED_FLAG_ACTIVATE_REQ));
    
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_config_p);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
        fbe_base_config_unlock(base_config_p);
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Clearing the clustered activate condition\n");
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }
    else
    {
        /* The fact that peer still has set the REQ flag means it has not yet seen that we have set the STARTED 
         * flag. The STARTED flag is an indication to the peer that we have seen it set the REQ flag. If we clear anything 
         * now the peer will incorrectly keep waiting for us to set the REQ flag.
         */
        fbe_base_config_unlock(base_config_p);
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Waiting for Peer to clear Req\n", __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
}

/*!****************************************************************************
 * fbe_base_config_check_hook()
 ******************************************************************************
 * @brief
 *   This function checks the hook status and returns the appropriate lifecycle status.
 *
 *
 * @param base_config_p              - pointer to the base config
 * @param monitor_state             - the monitor state
 * @param monitor_substate          - the substate of the monitor
 * @param val2                      - a generic value that can be used (e.g. checkpoint)
 * @param status                    - pointer to the status
 *
 * @return fbe_status_t   - FBE_STATUS_OK
 * @author
 *   01/31/12  - Created - Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_base_config_check_hook(fbe_base_config_t *base_config_p,
                                       fbe_u32_t monitor_state,
                                       fbe_u32_t monitor_substate,
                                       fbe_u64_t val2,
                                       fbe_scheduler_hook_status_t *status)
{
    fbe_status_t ret_status;
    ret_status = fbe_base_object_check_hook((fbe_base_object_t *)base_config_p,
                                         monitor_state,
                                         monitor_substate,
                                         val2,
                                         status);
    return ret_status;
}


/*!**************************************************************
 * fbe_base_config_npmetadata_upgrade_done_release_np_lock()
 ****************************************************************
 * @brief
 *  This is the completion function for writing the np metadata
 *  where we need to release the np dl.
 *
 * @param packet - packet ptr
 * @param context - non-paged metadata upgrade context ptr.
 *
 * @return fbe_status_t 
 *
 * @author
 *  4/18/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/

static fbe_status_t 
fbe_base_config_npmetadata_upgrade_done_release_np_lock(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_base_config_t *base_config_p = NULL;
    fbe_base_config_np_metadata_upgrade_context_t * np_upgrade_context = NULL;

    np_upgrade_context = (fbe_base_config_np_metadata_upgrade_context_t*)context;
    base_config_p = np_upgrade_context->base_config_p;
    fbe_base_config_release_np_distributed_lock(base_config_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_base_config_npmetadata_upgrade_done_release_np_lock()
 **************************************/

/*!**************************************************************
 * fbe_base_config_update_version_get_np_lock_completion()
 ****************************************************************
 * @brief
 *  This is the completion function for fetching the np distributed lock.
 *  Now we will complete writing the non-paged metadata with new version
 *  head by calling the non-paged metadata update and persist function.
 *
 * @param packet - packet ptr
 * @param context - base config ptr.
 *
 * @return fbe_status_t 
 *
 * @author
 *  4/18/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/

static fbe_status_t 
fbe_base_config_update_version_get_np_lock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_base_config_t *         base_config_p = NULL;
    fbe_status_t                packet_status = FBE_STATUS_OK;
    fbe_u64_t                   metadata_offset = 0;
    fbe_base_config_np_metadata_upgrade_context_t *np_upgrade_context = NULL;
    

    np_upgrade_context = (fbe_base_config_np_metadata_upgrade_context_t *)context;
    base_config_p = np_upgrade_context->base_config_p;
    /* Check if we get the NP lock successfully.
     */
    packet_status = fbe_transport_get_status_code(packet_p);
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: error %d on get np dl call\n", 
                              __FUNCTION__, packet_status);
        return FBE_STATUS_OK;
    }

    /*set a completion function to release the NP lock*/
    fbe_transport_set_completion_function(packet_p, fbe_base_config_npmetadata_upgrade_done_release_np_lock, np_upgrade_context);

    /* Now we are done getting the lock, update the version header and write the
       non-paged metadata.
     */
    metadata_offset = (fbe_u64_t) (&((fbe_base_config_nonpaged_metadata_t*)0)->version_header);    

    /*now update and persist the latest version header*/
    packet_status = fbe_base_config_metadata_nonpaged_write_persist(base_config_p, packet_p, 
                                                 metadata_offset, (fbe_u8_t*) &np_upgrade_context->version_header, 
                                                 sizeof(fbe_sep_version_header_t));
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "Error %d on write persist non-paged metadata version header\n", 
                              packet_status);
        fbe_transport_set_status(packet_p, packet_status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_base_config_update_version_get_np_lock_completion()
 **************************************/


/*!**************************************************************
 * fbe_base_config_update_version_get_np_lock_completion()
 ****************************************************************
 * @brief
 *  This is the completion function for fetching the np distributed lock.
 *  Now we will complete writing the non-paged metadata with new version
 *  head by calling the non-paged metadata update and persist function.
 *
 * @param packet - packet ptr
 * @param context - base config ptr.
 *
 * @return fbe_status_t 
 *
 * @author
 *  4/19/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/

static fbe_status_t 
fbe_base_config_update_version_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_base_config_t *         base_config_p = NULL;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_base_config_np_metadata_upgrade_context_t *np_upgrade_context = NULL;
    

    np_upgrade_context = (fbe_base_config_np_metadata_upgrade_context_t *)context;
    base_config_p = np_upgrade_context->base_config_p;

    /*no long need the memory, release it*/
    fbe_transport_release_buffer(np_upgrade_context);

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_config_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);
    } else {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Background Non-paged Metadta Version Updating Done obj %x \n",
                          base_config_p->base_object.object_id);
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_base_config_update_version_get_np_lock_completion()
 **************************************/


/*!**************************************************************
 * base_config_check_nonpaged_metadata_version_cond_function()
 ****************************************************************
 * @brief
 *  check if the version (size) between in-memory non-paged metadata and the disk
 *  data consistent.  if not consistent, change the size to current in-memory data
 *  structure size and persist it
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE
 * @author
 *  4/18/2012 - Created. Jingcheng Zhang
 ****************************************************************/
static fbe_lifecycle_status_t base_config_check_nonpaged_metadata_version_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_base_config_t *         base_config_p = (fbe_base_config_t *)object_p;
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                   committed_nonpaged_size = 0;
    fbe_bool_t                  is_active = FBE_FALSE;
    fbe_bool_t                  is_metadata_initialized = FBE_FALSE;
    fbe_base_config_nonpaged_metadata_t    *base_config_nonpaged_metadata_p;
    fbe_base_config_np_metadata_upgrade_context_t *upgrade_op_context = NULL;
    fbe_u64_t                   metadata_offset = 0;

    /*only check and upgrade on the active side when non-paged metadata valid*/
    is_active = fbe_base_config_is_active(base_config_p);
    if (!is_active){
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_config_p);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,"%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_config_get_nonpaged_metadata_ptr(base_config_p, (void **)&base_config_nonpaged_metadata_p);
    if (!base_config_nonpaged_metadata_p) {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_config_p);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,"%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /*check if the non-paged metadata is initialized, only check version after initialized*/
    fbe_base_config_metadata_nonpaged_is_initialized(base_config_p, &is_metadata_initialized);
    if (!is_metadata_initialized){
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_config_p);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,"%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /*see if the committed non-paged metadata size different from disk version size*/
    status = fbe_database_get_committed_nonpaged_metadata_size(object_p->class_id, &committed_nonpaged_size);
    if (status != FBE_STATUS_OK || base_config_nonpaged_metadata_p->version_header.size >= committed_nonpaged_size) {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_config_p);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,"%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_object_trace(object_p, FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                          "Background Non-paged Metadta Version Updating obj %x \n",
                          object_p->object_id);


    /*reach here means we need change non-paged metadata size, update peer and persist*/
    /*initialize the upgrade context*/
    upgrade_op_context = (fbe_base_config_np_metadata_upgrade_context_t *)fbe_transport_allocate_buffer();
    if (!upgrade_op_context){
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_config_p);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,"%s can't clear current condition, status: 0x%X\n",
                              __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    upgrade_op_context->base_config_p = base_config_p;

    /*set the correct version header with committed non-paged metadata size*/
    fbe_zero_memory(&upgrade_op_context->version_header, sizeof (fbe_sep_version_header_t));
    upgrade_op_context->version_header.size = committed_nonpaged_size;

    /*set completion function to release resource and clear condition as the final step*/
    fbe_transport_set_completion_function(packet_p, fbe_base_config_update_version_completion, upgrade_op_context);

    /*non-paged metadata may persist in either active or passive side, so grap the NP lock 
      before the update and persist; we reach here in case upgrade nonpaged metadata of LUN,
      Raid, VD and PVD. But for LUN, we will not get the NP lock and write/persist directly.
     */
    /*we judge sub class by its class ID here. will improve it if any better approach to check 
      whether NP lock required or not found*/
    if (base_config_p->base_object.class_id == FBE_CLASS_ID_LUN){
        metadata_offset = (fbe_u64_t) (&((fbe_base_config_nonpaged_metadata_t*)0)->version_header);    

        /*now update and persist the latest version header*/
        status = fbe_base_config_metadata_nonpaged_write_persist(base_config_p, packet_p, 
                                                 metadata_offset, (fbe_u8_t*) &upgrade_op_context->version_header, 
                                                 sizeof(fbe_sep_version_header_t));
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "Error %d on write persist non-paged metadata version header\n", 
                              status);
            fbe_transport_set_status(packet_p, status, 0);
            /*complete function set before will clear the condition*/
            fbe_transport_complete_packet(packet_p);
            return FBE_LIFECYCLE_STATUS_DONE; 
        }
    } else {
        /*completion function to persist after acquiring the lock*/
        fbe_transport_set_completion_function(packet_p, fbe_base_config_update_version_get_np_lock_completion, upgrade_op_context);
        
        fbe_base_config_get_np_distributed_lock(base_config_p, packet_p);
    }

    return FBE_LIFECYCLE_STATUS_PENDING;
}

fbe_status_t
fbe_base_config_monitor_crank_object(fbe_lifecycle_const_t * p_class_const,
                                    fbe_base_object_t * p_object,
                                    fbe_packet_t * packet)
{
    fbe_base_config_t * base_config = (fbe_base_config_t *)p_object;
    fbe_packet_resource_priority_t resource_priority;

    resource_priority = fbe_transport_get_resource_priority(packet); 

    if(resource_priority < 3){ /* Temporary base line */
        resource_priority = 3;
    }

    if(base_config->resource_priority >= resource_priority){
        resource_priority = base_config->resource_priority + 1;
    }

    /* Monitor packet should have higher priority than queued I/O 's */
    fbe_transport_set_resource_priority(packet, resource_priority);

    return fbe_lifecycle_crank_object(p_class_const, p_object, packet);
}

/*******************************
 * end fbe_base_config_monitor.c
 *******************************/
