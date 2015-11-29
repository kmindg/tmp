/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_extent_pool_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the extent_pool_p object lifecycle code.
 * 
 *  This includes the
 *  @ref fbe_extent_pool_monitor_entry "extent_pool_p monitor entry point", as well as all
 *  the lifecycle defines such as rotaries and conditions, along with the actual condition
 *  functions.
 * 
 * @ingroup extent_pool_class_files
 * 
 * @version
 *   06/06/2014:  Created.
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library.h"
#include "fbe/fbe_winddk.h"
//#include "fbe/fbe_provision_drive.h"
#include "fbe_extent_pool_private.h"
#include "fbe_cmi.h"
#include "fbe_job_service.h" 
#include "fbe_transport_memory.h"
#include "fbe_database.h"
#include "fbe_notification.h"
#include "fbe_base_config_private.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_event_log_utils.h"

/*!***************************************************************
 * fbe_extent_pool_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the extent_pool_p's monitor.
 *
 * @param object_handle - This is the object handle, or in our case the pdo.
 * @param packet_p - The packet_p arriving from the monitor scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.             
 *
 * @author
 *   06/06/2014:  Created.
 *
 ****************************************************************/

fbe_status_t 
fbe_extent_pool_monitor_entry(fbe_object_handle_t object_handle, 
                                  fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_extent_pool_t * extent_pool_p = NULL;

    extent_pool_p = (fbe_extent_pool_t *)fbe_base_handle_to_pointer(object_handle);

    status = fbe_base_config_monitor_crank_object(&fbe_extent_pool_lifecycle_const,
                                        (fbe_base_object_t*)extent_pool_p, packet_p);
    return status;
}
/******************************************
 * end fbe_extent_pool_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_extent_pool_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the extent_pool_p.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the extent_pool_p's constant
 *                        lifecycle data.
 *
 * @author
 *   06/06/2014:  Created.
 *
 ****************************************************************/

fbe_status_t 
fbe_extent_pool_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(extent_pool));
}
/******************************************
 * end fbe_extent_pool_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t fbe_extent_pool_metadata_memory_init_cond_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_extent_pool_metadata_memory_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t fbe_extent_pool_nonpaged_metadata_init_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_extent_pool_nonpaged_metadata_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t fbe_extent_pool_metadata_element_init_cond_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_extent_pool_metadata_initialize_metadata_element(fbe_extent_pool_t * extent_pool_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_extent_pool_metadata_element_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t fbe_extent_pool_stripe_lock_start_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_extent_pool_stripe_lock_start_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t fbe_extent_pool_write_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_extent_pool_write_default_nonpaged_metadata_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t fbe_extent_pool_persist_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_extent_pool_metadata_verify_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_extent_pool_peer_death_release_sl_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p);

static fbe_lifecycle_status_t fbe_extent_pool_init_pool_cond_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t fbe_extent_pool_downstream_health_not_optimal_cond_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_extent_pool_attach_edge(fbe_extent_pool_t * extent_pool_p, 
                                                fbe_u32_t edge_index, 
                                                fbe_object_id_t server_id, 
                                                fbe_packet_t * packet_p);


/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(extent_pool);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(extent_pool);



/*  extent_pool_lifecycle_callbacks
 *  This variable has all the callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(extent_pool) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        extent_pool,
        FBE_LIFECYCLE_NULL_FUNC,             /* online function */
        FBE_LIFECYCLE_NULL_FUNC /*fbe_extent_pool_pending_func*/)    /* pending function */
};


/*--- constant derived condition entries -----------------------------------------------*/

static fbe_lifecycle_const_cond_t extent_pool_metadata_memory_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_MEMORY_INIT,
        fbe_extent_pool_metadata_memory_init_cond_function)
};

static fbe_lifecycle_const_cond_t extent_pool_nonpaged_metadata_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_NONPAGED_METADATA_INIT,
        fbe_extent_pool_nonpaged_metadata_init_cond_function)         
};

static fbe_lifecycle_const_cond_t extent_pool_metadata_element_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_ELEMENT_INIT,
        fbe_extent_pool_metadata_element_init_cond_function)         
};

static fbe_lifecycle_const_cond_t extent_pool_stripe_lock_start_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_STRIPE_LOCK_START,
        fbe_extent_pool_stripe_lock_start_cond_function)
};

static fbe_lifecycle_const_cond_t extent_pool_downstream_health_not_optimal_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_NOT_OPTIMAL,
        fbe_extent_pool_downstream_health_not_optimal_cond_function)
};

static fbe_lifecycle_const_cond_t extent_pool_write_default_nonpaged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_NONPAGED_METADATA,
        fbe_extent_pool_write_default_nonpaged_metadata_cond_function)         
};

static fbe_lifecycle_const_cond_t extent_pool_persist_default_nonpaged_metadata_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_PERSIST_DEFAULT_NONPAGED_METADATA,
        fbe_extent_pool_persist_default_nonpaged_metadata_cond_function)
};

static fbe_lifecycle_const_cond_t extent_pool_metadata_verify_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_VERIFY,
        fbe_extent_pool_metadata_verify_cond_function)
};

static fbe_lifecycle_const_cond_t extent_pool_peer_death_release_sl_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_PEER_DEATH_RELEASE_SL,
        fbe_extent_pool_peer_death_release_sl_function)
};


/*--- constant base condition entries --------------------------------------------------*/
/*
 * discover drive condition entry
 */
static fbe_lifecycle_const_base_cond_t extent_pool_init_pool_cond =
{
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "extent_pool_init_pool_cond",
        FBE_EXTENT_POOL_LIFECYCLE_COND_INIT_POOL,
        fbe_extent_pool_init_pool_cond_function),

    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE   */
        FBE_LIFECYCLE_STATE_READY,               /* READY      */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE  */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE    */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL       */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY    */
};


/* extent_pool_lifecycle_base_cond_array
 * This is our static list of base conditions.
 */
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(extent_pool)[] =
{
    &extent_pool_init_pool_cond,
};

/* extent_pool_lifecycle_base_conditions 
 *  This is the list of base default base conditions for the 
 *  extent pool object.
 */
FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(extent_pool);


/*--- constant rotaries ----------------------------------------------------------------*/
static fbe_lifecycle_const_rotary_cond_t extent_pool_specialize_rotary[] = {
    /* Derived conditions */ 
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(extent_pool_metadata_memory_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(extent_pool_nonpaged_metadata_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(extent_pool_metadata_element_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(extent_pool_stripe_lock_start_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(extent_pool_downstream_health_not_optimal_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), 

    /*!@note following four condition needs to be in order, do not change the order without valid reason */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(extent_pool_write_default_nonpaged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(extent_pool_persist_default_nonpaged_metadata_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(extent_pool_metadata_verify_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(extent_pool_init_pool_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

#if 0
static fbe_lifecycle_const_rotary_cond_t extent_pool_activate_rotary[] = {
    /* Derived conditions */
    
    /* Base conditions */    
};
#endif

static fbe_lifecycle_const_rotary_cond_t extent_pool_ready_rotary[] =
{
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(extent_pool_peer_death_release_sl_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

    /* Base conditions */
};

#if 0
static fbe_lifecycle_const_rotary_cond_t extent_pool_fail_rotary[] = {
    /* Derived conditions */

    /* Base conditions */
};

static fbe_lifecycle_const_rotary_cond_t extent_pool_hibernate_rotary[] = {
    /* Derived conditions */

    /* Base conditions */
};

static fbe_lifecycle_const_rotary_cond_t extent_pool_destroy_rotary[] = {
    /* Derived conditions */

    /* Base conditions */
};
#endif

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(extent_pool)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, extent_pool_specialize_rotary),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, extent_pool_activate_rotary), 
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, extent_pool_ready_rotary),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_FAIL, extent_pool_fail_rotary),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_HIBERNATE, extent_pool_hibernate_rotary),
    //FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, extent_pool_destroy_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(extent_pool);


/*--- global extent_pool_p lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(extent_pool) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        extent_pool,
        FBE_CLASS_ID_EXTENT_POOL,            /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_config))  /* The super class */
};


/*--- Condition Functions --------------------------------------------------------------*/

/*!****************************************************************************
 * fbe_extent_pool_metadata_memory_init_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the metadata memory information for the
 *  RAID object.
 *
 * @param in_object_p               - pointer to a base object 
 * @param in_packet_p               - pointer to a control packet from the scheduler.
 *
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when metadata
 *                                    memory init issued.
 *
 * @author
 *  06/17/2014 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_extent_pool_metadata_memory_init_cond_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p)
{
    fbe_extent_pool_t *     extent_pool_p = (fbe_extent_pool_t *)base_object_p;
    
    fbe_base_object_trace(base_object_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Set the completion function for the metadata memory initialization. */
    fbe_transport_set_completion_function(packet_p, fbe_extent_pool_metadata_memory_init_cond_completion, extent_pool_p);

    /* Initialize the metadata memory for the RAID group object. */
    fbe_base_config_metadata_init_memory((fbe_base_config_t *) extent_pool_p, 
                                         packet_p,
                                         &extent_pool_p->extent_pool_metadata_memory,
                                         &extent_pool_p->extent_pool_metadata_memory_peer,
                                         sizeof(fbe_extent_pool_metadata_memory_t));
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_extent_pool_metadata_memory_init_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_extent_pool_metadata_memory_init_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the metadata memory
 *  initialization.
 *
 * @param packet_p      - pointer to a control packet from the scheduler.
 * @param context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  06/17/2014 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_extent_pool_metadata_memory_init_cond_completion(fbe_packet_t * packet_p,
                                                    fbe_packet_completion_context_t context)
{
    fbe_extent_pool_t *                  extent_pool_p = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    extent_pool_p = (fbe_extent_pool_t*)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code(packet_p);

    /* Get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* If status is good then clear the metadata memory initialization
     * condition. 
     */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* Clear the metadata memory initialization condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)extent_pool_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X\n",
                                    __FUNCTION__, status);
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_extent_pool_metadata_memory_init_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_extent_pool_nonpaged_metadata_init()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the nonpaged metadata memory of the
 *  provision drive object.
 *
 * @param extent_pool_p         - pointer to the provision drive
 * @param packet_p                  - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  06/17/2014 - Create. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_extent_pool_nonpaged_metadata_init(fbe_extent_pool_t * extent_pool_p,
                                           fbe_packet_t *packet_p)
{
    fbe_status_t status;

    /* call the base config nonpaged metadata init to initialize the metadata. */
    status = fbe_base_config_metadata_nonpaged_init((fbe_base_config_t *) extent_pool_p,
                                                    sizeof(fbe_extent_pool_nonpaged_metadata_t),
                                                    packet_p);
    return status;
}
/******************************************************************************
 * end fbe_extent_pool_nonpaged_metadata_init()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_extent_pool_nonpaged_metadata_init_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the extent pool object's nonpaged
 *  metadata memory.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  06/17/2014 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_extent_pool_nonpaged_metadata_init_cond_function(fbe_base_object_t * object_p,
                                                         fbe_packet_t * packet_p)
{
    fbe_extent_pool_t *         extent_pool_p = NULL;
    fbe_status_t                    status;

    extent_pool_p = (fbe_extent_pool_t *)object_p;

    /* set the completion function for the nonpaged metadata init condition. */
    fbe_transport_set_completion_function(packet_p, fbe_extent_pool_nonpaged_metadata_init_cond_completion, extent_pool_p);

    /* initialize signature for the extent pool object. */
    status = fbe_extent_pool_nonpaged_metadata_init(extent_pool_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_extent_pool_nonpaged_metadata_init_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_extent_pool_nonpaged_metadata_init_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the nonpaged metadata init
 *  condition.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  06/17/2014 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_extent_pool_nonpaged_metadata_init_cond_completion(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context)
{
    fbe_extent_pool_t *             extent_pool_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    extent_pool_p = (fbe_extent_pool_t*)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* clear nonpaged metadata init condition if status is good. */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) extent_pool_p);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)extent_pool_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s nonpaged metadata init condition failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_extent_pool_nonpaged_metadata_init_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_extent_pool_metadata_element_init_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the metadata element for the RAID group
 *  object.
 *
 * @param base_object_p             - pointer to a base object.
 * @param packet_p                  - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when signature
 *                                    verification issued.
 *
 * @author
 *  06/17/2014 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_extent_pool_metadata_element_init_cond_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p)
{
    fbe_extent_pool_t * extent_pool_p = NULL;

    extent_pool_p = (fbe_extent_pool_t*)base_object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Set the completion function for the metadata record initialization. */
    fbe_transport_set_completion_function(packet_p, fbe_extent_pool_metadata_element_init_cond_completion, extent_pool_p);

    /* Initialiez the raid group metadata record. */
    fbe_extent_pool_metadata_initialize_metadata_element(extent_pool_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_extent_pool_metadata_element_init_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_extent_pool_metadata_initialize_metadata_element()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the metadata record for the RAID
 *   group object.
 *
 * @param extent_pool_p         - pointer to the raid group
 * @param packet_p             - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  06/17/2014 - Create. Lili Chen
 *
 ******************************************************************************/
fbe_status_t
fbe_extent_pool_metadata_initialize_metadata_element(fbe_extent_pool_t * extent_pool_p,
                                                    fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                     payload_p = NULL;
    fbe_payload_control_operation_t *      control_operation_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

#if 0
    /* initialize metadata element with metadata position information. */
    fbe_base_config_metadata_set_paged_record_start_lba((fbe_base_config_t *) extent_pool_p,
                                                        metadata_positions.paged_metadata_lba);

    fbe_base_config_metadata_set_paged_mirror_metadata_offset((fbe_base_config_t *) extent_pool_p,
                                                              metadata_positions.paged_mirror_metadata_offset);

    fbe_base_config_metadata_set_paged_metadata_capacity((fbe_base_config_t *) extent_pool_p,
                                                        metadata_positions.paged_metadata_capacity);

    fbe_base_config_metadata_set_number_of_stripes((fbe_base_config_t *) extent_pool_p,
                                                   number_of_stripes);
    fbe_base_config_metadata_set_number_of_private_stripes((fbe_base_config_t *) extent_pool_p,
                                                            ( number_of_stripes - number_of_user_stripes));
#endif

    /* complete the packet with good status here. */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_extent_pool_metadata_initialize_metadata_element()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_extent_pool_metadata_element_init_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the metadata element 
 *  initialization condition.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  06/17/2014 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_extent_pool_metadata_element_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_extent_pool_t *                 extent_pool_p = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t status;
    
    extent_pool_p = (fbe_extent_pool_t*)context;

    /* Get the status of the packet. */
    status = fbe_transport_get_status_code(packet_p);

    /* Get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* Clear the condition if status is ok. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {    
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*) extent_pool_p);
        if (status != FBE_STATUS_OK){
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X\n",
                                    __FUNCTION__, status);
        }

        fbe_topology_clear_gate(extent_pool_p->base_config.base_object.object_id);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_extent_pool_metadata_element_init_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_extent_pool_stripe_lock_start_cond_function()
 ******************************************************************************
 * @brief
 *  Will start stripe locking.
 *
 * @param object_p - current object.
 * @param packet_p - monitor packet
 *
 * @return fbe_lifecycle_status_t - FBE_LIFECYCLE_STATUS_DONE    
 *
 * @author
 *  06/17/2014 - Create. Lili Chen
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
fbe_extent_pool_stripe_lock_start_cond_function(fbe_base_object_t * object_p, 
                                               fbe_packet_t * packet_p)
{   
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_stripe_lock_operation_t * stripe_lock_operation = NULL;
    fbe_extent_pool_t * extent_pool_p = (fbe_extent_pool_t *)object_p;

    /* Do nothing if it is re-specializing.
     */
    if(fbe_base_config_is_flag_set((fbe_base_config_t *) object_p, FBE_BASE_CONFIG_FLAG_RESPECIALIZE))
    {
        fbe_lifecycle_clear_current_cond(object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }    

    if(fbe_base_config_stripe_lock_is_started((fbe_base_config_t *) object_p)){
        status = fbe_lifecycle_clear_current_cond(object_p);
        if (status != FBE_STATUS_OK){
            fbe_base_object_trace(object_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    //if(fbe_base_config_is_active((fbe_base_config_t *)object_p)){
        payload = fbe_transport_get_payload_ex(packet_p);
        stripe_lock_operation = fbe_payload_ex_allocate_stripe_lock_operation(payload);
        fbe_payload_stripe_lock_build_start(stripe_lock_operation, &((fbe_base_config_t *)object_p)->metadata_element);

        /* We may choose to allocate the memory here, but for now ... */
        ((fbe_base_config_t *)object_p)->metadata_element.stripe_lock_blob = &extent_pool_p->stripe_lock_blob;
        extent_pool_p->stripe_lock_blob.size = METADATA_STRIPE_LOCK_SLOTS;

        fbe_payload_ex_increment_stripe_lock_operation_level(payload);
        fbe_transport_set_completion_function(packet_p, fbe_extent_pool_stripe_lock_start_cond_completion, object_p);
        fbe_ext_pool_lock_operation_entry(packet_p);
        return FBE_LIFECYCLE_STATUS_PENDING;
    //} 

    //return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_extent_pool_stripe_lock_start_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_extent_pool_stripe_lock_start_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the metadata element 
 *  initialization condition.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  06/17/2014 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_extent_pool_stripe_lock_start_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_base_config_t * base_config = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_stripe_lock_operation_t * stripe_lock_operation = NULL;

    base_config = (fbe_base_config_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    stripe_lock_operation = fbe_payload_ex_get_stripe_lock_operation(payload);
    fbe_payload_ex_release_stripe_lock_operation(payload, stripe_lock_operation);

    if(fbe_base_config_stripe_lock_is_started(base_config)){
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *)base_config);
        if (status != FBE_STATUS_OK){
            fbe_base_object_trace((fbe_base_object_t *)base_config, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s can't clear current condition, status: 0x%x\n",
                                  __FUNCTION__, status);
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_extent_pool_stripe_lock_start_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_extent_pool_downstream_health_not_optimal_cond_function()
 ******************************************************************************
 * @brief
 *  This is the derived condition function for extent pool object where it 
 *  will hold the object in specialize state until it finds downstream health 
 *  is optimal.
 *
 * @param base_object_p          - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  06/06/2014 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_extent_pool_downstream_health_not_optimal_cond_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p)
{
    fbe_status_t            overall_status = FBE_STATUS_OK;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_extent_pool_t *     extent_pool_p = (fbe_extent_pool_t *)base_object_p;
    fbe_object_id_t         pool_object_id;
    fbe_object_id_t        *pvd_list = NULL;
    fbe_u32_t               drive_count;
    fbe_u32_t               pool_id = FBE_POOL_ID_INVALID;
    fbe_u32_t               i;
    fbe_block_edge_t *      block_edge_p;
  
    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s \n", __FUNCTION__);

    fbe_base_object_get_object_id(base_object_p, &pool_object_id);
    status = fbe_database_get_ext_pool_configuration_info(pool_object_id, &pool_id, &drive_count, NULL);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s DS objects doesn't exist yet\n", __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    /* Allocate list of PVDs.
     */
    pvd_list = (fbe_object_id_t*)fbe_memory_native_allocate(sizeof(fbe_object_id_t) * drive_count);
    if (pvd_list == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s cannot allocate pvd list.\n", __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    status = fbe_database_get_ext_pool_configuration_info(pool_object_id, &pool_id, &drive_count, pvd_list);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s DS objects doesn't exist yet\n", __FUNCTION__);
        fbe_memory_native_release(pvd_list);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    /* Fake capacity */
    //fbe_base_config_set_capacity((fbe_base_config_t*)extent_pool_p, 0x100000);

    fbe_base_config_set_width((fbe_base_config_t *)extent_pool_p, drive_count);

    /* Allocate memory for edges */
    fbe_base_config_get_block_edge_ptr((fbe_base_config_t*)extent_pool_p, &block_edge_p);
    if (block_edge_p == NULL)
    {
	    block_edge_p = fbe_memory_allocate_required(sizeof(fbe_block_edge_t) * drive_count);
        for (i = 0; i < drive_count; i++) {
            fbe_base_edge_init(&block_edge_p[i].base_edge);
        }
        fbe_base_config_set_block_edge_ptr((fbe_base_config_t *)extent_pool_p, block_edge_p);
    }

    /* Attach edges */
    for (i = 0; i < drive_count; i++)
    {
        status = fbe_extent_pool_attach_edge(extent_pool_p, i, pvd_list[i], packet_p);
        if (status != FBE_STATUS_OK) {
            overall_status = status;
        }
    }
    if (overall_status != FBE_STATUS_OK) {
        fbe_memory_native_release(pvd_list);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    /* clear the condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *) extent_pool_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X\n", __FUNCTION__, status);
    }

    fbe_memory_native_release(pvd_list);
    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_extent_pool_downstream_health_not_optimal_cond_function()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_extent_pool_metadata_write_default_nonpaged_metadata()
 ******************************************************************************
 * @brief
 *   This function is used to initialize the metadata with default values for 
 *   the non paged metadata.
 *
 * @param extent_pool_p        - pointer to the extent pool
 * @param packet_p             - pointer to a monitor packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  06/20/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t
fbe_extent_pool_metadata_write_default_nonpaged_metadata(fbe_extent_pool_t * extent_pool_p,
                                                        fbe_packet_t * packet_p)
{
    fbe_status_t                        status;
    fbe_bool_t                          b_is_active;
    fbe_extent_pool_nonpaged_metadata_t extent_pool_nonpaged_metadata;

    /*! @note We should not be here if this is not the active SP.
     */
    b_is_active = fbe_base_config_is_active((fbe_base_config_t *)extent_pool_p);
    if (b_is_active == FBE_FALSE)
    {
        fbe_base_object_trace((fbe_base_object_t *)extent_pool_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Unexpected passive set.\n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set up the defaults for the non-paged metadata.   First zero out the 
     * structure to write to it.  We'll explicitly set only those fields whose 
     * default values are non-zero.
     */
    fbe_zero_memory(&extent_pool_nonpaged_metadata, sizeof(fbe_extent_pool_nonpaged_metadata_t));

    /* Now set the default non-paged metadata.
     */
    status = fbe_base_config_set_default_nonpaged_metadata((fbe_base_config_t *)extent_pool_p,
                                                           (fbe_base_config_nonpaged_metadata_t *)&extent_pool_nonpaged_metadata);
    if (status != FBE_STATUS_OK)
    {
        /* Trace an error and return.
         */
        fbe_base_object_trace((fbe_base_object_t *)extent_pool_p,
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Set default base config nonpaged failed status: 0x%x\n",
                              __FUNCTION__, status);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return status;
    }	

    //  Send the nonpaged metadata write request to metadata service. 
    status = fbe_base_config_metadata_nonpaged_write((fbe_base_config_t *) extent_pool_p,
                                                 packet_p,
                                                 0, // offset is zero for the write of entire non paged metadata record. 
                                                 (fbe_u8_t *) &extent_pool_nonpaged_metadata, // non paged metadata memory. 
                                                 sizeof(fbe_extent_pool_nonpaged_metadata_t)); // size of the non paged metadata. 
    return status;
}
/******************************************************************************
 * end fbe_extent_pool_metadata_write_default_nonpaged_metadata()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_extent_pool_write_default_nonpaged_metadata_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to write the default nonpaged metadata for the 
 *  raid group object during object initialization.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  06/20/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_extent_pool_write_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p,
                                                              fbe_packet_t * packet_p)
{
    fbe_extent_pool_t *         extent_pool_p = NULL;
    fbe_status_t                    status;

    extent_pool_p = (fbe_extent_pool_t*)object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* set the completion function for the paged metadata init condition. */
    fbe_transport_set_completion_function(packet_p, fbe_extent_pool_write_default_nonpaged_metadata_cond_completion, extent_pool_p);

    /* initialize signature for the raid group object. */
    status = fbe_extent_pool_metadata_write_default_nonpaged_metadata(extent_pool_p, packet_p);
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_extent_pool_write_default_nonpaged_metadata_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_extent_pool_write_default_nonpaged_metadata_cond_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the signature init
 *  condition.
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  06/20/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_extent_pool_write_default_nonpaged_metadata_cond_completion(fbe_packet_t * packet_p,
                                                                    fbe_packet_completion_context_t context)
{
    fbe_extent_pool_t *             extent_pool_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    extent_pool_p = (fbe_extent_pool_t*)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        /* clear nonpaged metadata write default condition if status is good. */
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) extent_pool_p);

    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)extent_pool_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s write default nonpaged metadata condition failed, status:0x%X, control_status: 0x%X\n",
                              __FUNCTION__, status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_extent_pool_write_default_nonpaged_metadata_cond_completion()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_extent_pool_persist_default_nonpaged_metadata_cond_function()
 ******************************************************************************
 *
 * @brief   This function is used to persist the default nonpaged metadata for the 
 *          raid group object during object initialization.
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when init
 *                                    signature or metadata issued.
 * @author
 *  06/20/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_lifecycle_status_t fbe_extent_pool_persist_default_nonpaged_metadata_cond_function(fbe_base_object_t * object_p, 
                                                                                             fbe_packet_t * packet_p)
{
    fbe_status_t status;

    fbe_base_object_trace((fbe_base_object_t*)object_p,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    if (!fbe_base_config_is_active((fbe_base_config_t *)object_p)) {
        fbe_lifecycle_clear_current_cond((fbe_base_object_t *) object_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* At this point, we just go ahead and write the fact that we are now initialized */
    status = fbe_base_config_set_nonpaged_metadata_state((fbe_base_config_t *)object_p, packet_p, FBE_NONPAGED_METADATA_INITIALIZED);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(object_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s set np metadata state failed, status: 0x%x\n",
                              __FUNCTION__, status);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************************************************
 * end fbe_extent_pool_persist_default_nonpaged_metadata_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_extent_pool_metadata_verify_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to verify the extent pool's metadata initialization.
 *  It verify that if non paged metadata is not initialized than it set multiple 
 *  conditions to initialize raid group's metadata.

 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t   - FBE_LIFECYCLE_STATUS_PENDING when metadata
 *                                    verification done.
 *
 * @author
 *  06/20/2014 - Created. Lili Chen
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_extent_pool_metadata_verify_cond_function(fbe_base_object_t * object, fbe_packet_t * packet_p)
{
    fbe_bool_t is_metadata_initialized;
    fbe_bool_t is_active;
    fbe_extent_pool_t * extent_pool_p = NULL;
    fbe_u32_t  disk_version_size = 0;
    fbe_u32_t  memory_version_size = 0;    
    fbe_base_config_nonpaged_metadata_t    *base_config_nonpaged_metadata_p = NULL;
    fbe_bool_t is_magic_num_valid;    

    extent_pool_p = (fbe_extent_pool_t*)object;  

    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry.\n", __FUNCTION__);

    /*check the NP magic number .
        If it is INVALID, the NP is not load correctly
        the object will stuck in this condition function until 
        its NP is set correctly by caller*/
    fbe_base_config_metadata_is_nonpaged_state_valid((fbe_base_config_t *) object, &is_magic_num_valid);
    if(is_magic_num_valid == FBE_FALSE)
    {
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    is_active = fbe_base_config_is_active((fbe_base_config_t *)extent_pool_p);
    fbe_base_config_metadata_is_initialized((fbe_base_config_t *)extent_pool_p, &is_metadata_initialized);

    /*version check.  the non-paged metadata on disk may have different version with current
      non-paged metadata structure in memory. compare the size and set default value for the
      new version in memory*/
    if (is_metadata_initialized) {
        fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)extent_pool_p, (void **)&base_config_nonpaged_metadata_p);
        fbe_base_config_get_nonpaged_metadata_size((fbe_base_config_t *)extent_pool_p, &memory_version_size);
        fbe_base_config_nonpaged_metadata_get_version_size(base_config_nonpaged_metadata_p, &disk_version_size);
        if (disk_version_size != memory_version_size) {
            /*new software is operating old version data during upgrade, set default value for the new version*/
            /*currently these region already zerod, the new version developer should set correct value here*/
            fbe_base_object_trace((fbe_base_object_t*)extent_pool_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "metatadata verify detect version un-match here, should set default value here. disk size %d, memory size %d\n",
                              disk_version_size, memory_version_size);            
        }

        /* Set flag nonpaged metadata is initialized */
        fbe_base_config_set_clustered_flag((fbe_base_config_t *)extent_pool_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_NONPAGED_INITIALIZED);
    }

    if (!is_active){ /* This will take care of passive side */
        if(is_metadata_initialized == FBE_TRUE){
            fbe_lifecycle_clear_current_cond((fbe_base_object_t *) extent_pool_p);
            fbe_base_config_clear_clustered_flag((fbe_base_config_t *) extent_pool_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST);
            fbe_lifecycle_clear_current_cond((fbe_base_object_t *) extent_pool_p);            
        } else {
            /* Ask again for the nonpaged metadata update */
            if(!fbe_base_config_is_clustered_flag_set((fbe_base_config_t *) extent_pool_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST)){
                fbe_base_config_set_clustered_flag((fbe_base_config_t *) extent_pool_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST);
            }
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }


    if (!is_metadata_initialized) { /* We active and newly created */
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s We active and newly created\n", __FUNCTION__);

        /* Set all relevant conditions */
        /* We are initializing the object first time and so update the 
         * non paged metadata, paged metadata and non paged metadata signature in order.
         */
         /* set the nonpaged metadata init condition. */
        fbe_lifecycle_set_cond(&fbe_extent_pool_lifecycle_const,
                               (fbe_base_object_t *)extent_pool_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_NONPAGED_METADATA);

#if 0
        /* set the zero consumed area condition. */
        fbe_lifecycle_set_cond(&fbe_extent_pool_lifecycle_const,
                               (fbe_base_object_t *)extent_pool_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_ZERO_CONSUMED_AREA);

        /* set the paged metadata init condition. */
       fbe_lifecycle_set_cond(&fbe_extent_pool_lifecycle_const,
                              (fbe_base_object_t *)extent_pool_p,
                              FBE_BASE_CONFIG_LIFECYCLE_COND_WRITE_DEFAULT_PAGED_METADATA);
#endif

        fbe_lifecycle_set_cond(&fbe_extent_pool_lifecycle_const,
                               (fbe_base_object_t *)extent_pool_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_PERSIST_DEFAULT_NONPAGED_METADATA);

        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_config_clear_clustered_flag((fbe_base_config_t *) extent_pool_p, FBE_BASE_CONFIG_METADATA_MEMORY_PEER_NONPAGED_REQUEST);

    /* clear the metadata verify condition */
    fbe_lifecycle_clear_current_cond((fbe_base_object_t *) extent_pool_p);

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_extent_pool_metadata_verify_cond_function()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_extent_pool_peer_death_release_sl_function()
 ******************************************************************************
 * @brief
 *  This function is used to verify the drive fault bit of the pvd.
 *
 * @param base_object_p          - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  06/25/2012 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_extent_pool_peer_death_release_sl_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_extent_pool_t *         extent_pool_p = (fbe_extent_pool_t *)base_object_p;
    

    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Release all SL \n ", __FUNCTION__);
    fbe_ext_pool_lock_release_peer_data_stripe_locks(&extent_pool_p->base_config.metadata_element);

    fbe_metadata_element_clear_abort_monitor_ops(&((fbe_base_config_t *)extent_pool_p)->metadata_element);

    /* clear the condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *) extent_pool_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X", __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_extent_pool_peer_death_release_sl_function()
 ******************************************************************************/


/*!**************************************************************
 * fbe_extent_pool_init_hash_table()
 ****************************************************************
 * @brief
 *  Allocate and init the hash table.
 *
 * @param extent_pool_p - Pool object.               
 *
 * @return fbe_status_t
 *
 * @author
 *  6/18/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_extent_pool_init_hash_table(fbe_extent_pool_t *extent_pool_p)
{
    fbe_u32_t hash_index;
    fbe_extent_pool_hash_table_t *hash_table_p = NULL;

    extent_pool_p->hash_table = fbe_memory_allocate_required(sizeof(fbe_extent_pool_hash_table_entry_t) * (fbe_u32_t)(extent_pool_p->total_slices));
    if (extent_pool_p->hash_table == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Cannot allocate memory for hash table\n", __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    hash_table_p = extent_pool_p->hash_table;

    /* Initialize hash table to empty.
     */
    for (hash_index = 0; hash_index < extent_pool_p->total_slices; hash_index++) {
        hash_table_p[hash_index].slice_ptr = NULL;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_init_hash_table()
 ******************************************/

/*!**************************************************************
 * fbe_extent_pool_init_disk_info()
 ****************************************************************
 * @brief
 *  Allocate and initialize the memory for the pool's slices.
 *
 * @param extent_pool_p - Pool object.               
 *
 * @return fbe_status_t
 *
 * @author
 *  6/11/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_extent_pool_init_disk_info(fbe_extent_pool_t *extent_pool_p)
{
    fbe_status_t status;
    fbe_extent_pool_disk_slice_t *slice_p = NULL;
    fbe_u32_t                    pool_width;
    fbe_object_id_t              pool_object_id;
    fbe_object_id_t             *pvd_list = NULL;
    fbe_u32_t                    pool_id = FBE_POOL_ID_INVALID;
    fbe_u32_t                    disk_index;
    fbe_extent_pool_disk_info_t *disk_info_p = NULL;
    fbe_slice_count_t            total_slices = 0;
    fbe_slice_count_t            disk_slices;
    fbe_slice_index_t            slice_index;
    fbe_extent_pool_disk_slice_t *disk_slice_memory_p = NULL;
    fbe_extent_pool_disk_slice_t *current_slice_p = NULL;
    fbe_lba_t                    lba;
    fbe_block_count_t            blocks_per_disk_slice;

    fbe_extent_pool_class_get_blocks_per_disk_slice(&blocks_per_disk_slice);

    fbe_base_object_get_object_id((fbe_base_object_t*)extent_pool_p, &pool_object_id);
    status = fbe_database_get_ext_pool_configuration_info(pool_object_id, &pool_id, &pool_width, NULL);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s DS objects doesn't exist yet\n", __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    /* Allocate list of PVDs.
     */
    pvd_list = (fbe_object_id_t*)fbe_memory_native_allocate(sizeof(fbe_object_id_t) * pool_width);
    if (pvd_list == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s cannot allocate pvd list.\n", __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    status = fbe_database_get_ext_pool_configuration_info(pool_object_id, &pool_id, &pool_width, pvd_list);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s DS objects doesn't exist yet\n", __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "Extent pool: initting pool %u with %d drives\n", pool_id, pool_width);

    disk_info_p = fbe_memory_allocate_required((fbe_u32_t)(sizeof(fbe_extent_pool_disk_info_t) * pool_width));
    if (disk_info_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Cannot allocate memory for disk info\n", __FUNCTION__);
        fbe_memory_native_release(pvd_list);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    extent_pool_p->disk_info_p = disk_info_p;

    /* Initialize the disk information for all drives.
     */
    for (disk_index = 0; disk_index < pool_width; disk_index++) {
        disk_info_p[disk_index].free_slice_index = 0;
        disk_info_p[disk_index].object_id = pvd_list[disk_index];
        fbe_database_get_pvd_capacity(pvd_list[disk_index], &disk_info_p[disk_index].capacity);
        total_slices += disk_info_p[disk_index].capacity / blocks_per_disk_slice;

        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Extent pool: Adding disk %u) object: 0x%x capacity: 0x%llx\n", 
                              disk_index, pvd_list[disk_index], disk_info_p[disk_index].capacity);
    }
    fbe_memory_native_release(pvd_list);
    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "Extent pool: allocating 0x%llx total disk slices\n", total_slices);

    if (total_slices == 0) {
        fbe_memory_release_required(disk_info_p);
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s 0 slices required for pool\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Allocate memory for all disk slices. We will distribute this to each disk below.
     */
    disk_slice_memory_p = fbe_memory_allocate_required((fbe_u32_t)(sizeof(fbe_extent_pool_disk_slice_t) * total_slices));
    if (disk_slice_memory_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Cannot allocate memory for disk slices\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Give each disk its slice memory.
     */
    current_slice_p = disk_slice_memory_p;
    for (disk_index = 0; disk_index < pool_width; disk_index++) {
        disk_slices = disk_info_p[disk_index].capacity / blocks_per_disk_slice;
        disk_info_p[disk_index].drive_map_table_p = current_slice_p;
        current_slice_p += disk_slices;
    }

    /* Initialize the disk slices.
     */
    for (disk_index = 0; disk_index < pool_width; disk_index++) {
        slice_p = disk_info_p[disk_index].drive_map_table_p;
        disk_slices = disk_info_p[disk_index].capacity / blocks_per_disk_slice;
        lba = FBE_EXTENT_POOL_START_OFFSET;
        for (slice_index = 0; slice_index < disk_slices; slice_index++) {
            slice_p->disk_address = lba;
            slice_p->extent_address = 0;
            slice_p++;
            lba += blocks_per_disk_slice;
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_init_disk_info()
 ******************************************/

/*!**************************************************************
 * fbe_extent_pool_initialize_pool_width()
 ****************************************************************
 * @brief
 *   Calculate the total number of blocks and slices in the pool.  
 *
 * @param extent_pool_p - Current pool.               
 *
 * @return fbe_status_t
 *
 * @author
 *  6/24/2014 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t 
fbe_extent_pool_initialize_pool_width(fbe_extent_pool_t *extent_pool_p)
{
    fbe_status_t            status;
    fbe_u32_t               pool_width;
    fbe_object_id_t         pool_object_id;
    fbe_u32_t               pool_id = FBE_POOL_ID_INVALID;
  
    fbe_base_object_get_object_id((fbe_base_object_t*)extent_pool_p, &pool_object_id);
    status = fbe_database_get_ext_pool_configuration_info(pool_object_id, &pool_id, &pool_width, NULL);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s DS objects doesn't exist yet\n", __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    extent_pool_p->pool_width = pool_width;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_extent_pool_initialize_pool_width() 
 **************************************/
/*!**************************************************************
 * fbe_extent_pool_calculate_pool_blocks()
 ****************************************************************
 * @brief
 *   Calculate the total number of blocks and slices in the pool.  
 *
 * @param extent_pool_p - Current pool.               
 *
 * @return fbe_status_t
 *
 * @author
 *  6/24/2014 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t 
fbe_extent_pool_calculate_pool_blocks(fbe_extent_pool_t *extent_pool_p)
{
    fbe_slice_count_t            total_disk_slices = 0;
    fbe_slice_count_t            total_slices = 0;
    fbe_u32_t                    pool_width;
    fbe_block_count_t            total_capacity;
    fbe_extent_pool_disk_info_t *disk_info_p = NULL;
    fbe_block_count_t            blocks_per_slice;
    fbe_block_count_t            blocks_per_disk_slice;
    fbe_u32_t                    disk_index;
    fbe_slice_count_t            disk_slices;

    fbe_extent_pool_get_pool_width(extent_pool_p, &pool_width);
    fbe_extent_pool_class_get_blocks_per_slice(&blocks_per_slice);
    fbe_extent_pool_class_get_blocks_per_disk_slice(&blocks_per_disk_slice);

    /* Total up all slices on the drives.
     * Note that this does not take into account drives of different capacities. 
     */
    for (disk_index = 0; disk_index < pool_width; disk_index++) {
        fbe_extent_pool_get_disk_info(extent_pool_p, disk_index, &disk_info_p);
        disk_slices = disk_info_p->capacity / blocks_per_disk_slice;
        total_disk_slices += disk_slices;
    }

    /* Determine the number of usable slices by taking parity into account.
     */
    total_slices = (total_disk_slices / FBE_EXTENT_POOL_DEFAULT_WIDTH);

    total_capacity = total_slices * blocks_per_slice * FBE_EXTENT_POOL_DEFAULT_DATA_DISKS;
    fbe_base_config_set_capacity((fbe_base_config_t*)extent_pool_p, total_capacity);

    extent_pool_p->total_slices = total_slices;
    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "extent pool: total_disk_slices: 0x%llx total slices: 0x%llx total_capacity: 0x%llx\n",
                          total_disk_slices, total_slices, total_capacity);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_calculate_pool_blocks()
 ******************************************/
/*!**************************************************************
 * fbe_extent_pool_init_geometries()
 ****************************************************************
 * @brief
 *  Allocate and initialize all the geometry objects in use by
 *  the pool.
 *
 * @param extent_pool_p - Pool object.               
 *
 * @return fbe_status_t
 *
 * @author
 *  6/11/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_extent_pool_init_geometries(fbe_extent_pool_t *extent_pool_p)
{
    fbe_status_t            status;
    fbe_raid_geometry_t    *raid_geometry_p = NULL;
    fbe_block_edge_t       *edge_p = NULL;
    fbe_u32_t               max_bytes_per_drive_request;
    fbe_u32_t               max_sg_entries;
    fbe_u32_t               pool_width;
    fbe_object_id_t         pool_object_id;
    fbe_u32_t               pool_id = FBE_POOL_ID_INVALID;
  
    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s \n", __FUNCTION__);
    
    fbe_extent_pool_get_pool_width(extent_pool_p, &pool_width);
    fbe_base_object_get_object_id((fbe_base_object_t*)extent_pool_p, &pool_object_id);
    status = fbe_database_get_ext_pool_configuration_info(pool_object_id, &pool_id, &pool_width, NULL);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s DS objects doesn't exist yet\n", __FUNCTION__);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    extent_pool_p->geometry_p = fbe_memory_allocate_required((fbe_u32_t)(sizeof(fbe_raid_geometry_t) * FBE_EXTENT_POOL_GEOMETRIES));
    if (extent_pool_p->geometry_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Cannot allocate memory for pool geometry\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&extent_pool_p->geometry_p[0], sizeof(fbe_raid_geometry_t) * FBE_EXTENT_POOL_GEOMETRIES);
    fbe_base_config_set_width((fbe_base_config_t *)extent_pool_p, pool_width);
    fbe_base_config_get_block_edge((fbe_base_config_t *)extent_pool_p, &edge_p, 0);

    /* Init the first one for the user slices.
     */
    raid_geometry_p = extent_pool_p->geometry_p;

    status = fbe_raid_geometry_init(raid_geometry_p,
                           extent_pool_p->base_config.base_object.object_id, /* Object id to use. */
                           FBE_CLASS_ID_PARITY,
                           NULL, /* No metadata element. */
                           edge_p, /* ptr to edge array */
                           (fbe_raid_common_state_t)fbe_parity_generate_start);

    if (status != FBE_STATUS_OK) { 
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s cannot set configuration status: 0x%x\n", __FUNCTION__, status);
        return status;
    }

    /* Need to issue control request to get maximum number of bytes
     * per drive request.
     */
    status = fbe_database_get_drive_transfer_limits(&max_bytes_per_drive_request, &max_sg_entries);

    if (status != FBE_STATUS_OK) { 
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s cannot get transfer limits status: 0x%x\n", __FUNCTION__, status);
        return status;
    }

    /*! @todo This is all hard coded for the prototype.
     */
    status = fbe_raid_geometry_set_configuration(raid_geometry_p,
                                                 FBE_EXTENT_POOL_DEFAULT_WIDTH, /* Width. */
                                                 FBE_RAID_GROUP_TYPE_RAID5,
                                                 FBE_RAID_SECTORS_PER_ELEMENT,
                                                 FBE_RAID_ELEMENTS_PER_PARITY,
                                                 FBE_LBA_INVALID, /* Just max for now. */
                                                 (max_bytes_per_drive_request/FBE_BE_BYTES_PER_BLOCK));
    if (status != FBE_STATUS_OK) { 
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s cannot set configuration status: 0x%x\n", __FUNCTION__, status);
    }
    fbe_raid_geometry_set_attribute(raid_geometry_p, FBE_RAID_ATTRIBUTE_EXTENT_POOL);
    /* We do not want to perform write logging with extent pools.
     */
    fbe_raid_geometry_set_debug_flag(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_WRITE_LOGGING);
#if 0
    fbe_raid_geometry_set_debug_flag(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING | 
                                                      FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING);
#endif
    status = fbe_raid_geometry_set_block_sizes(raid_geometry_p,
                                               FBE_BE_BYTES_PER_BLOCK,    /* Exported block size */
                                               FBE_BE_BYTES_PER_BLOCK,    /* Physical block size */
                                               1    /* Optimal size */);

    if (status != FBE_STATUS_OK) { 
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s cannot set block sizes status: 0x%x\n", __FUNCTION__, status);
        return status;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_init_geometries()
 ******************************************/

/*!****************************************************************************
 * fbe_extent_pool_init_pool_cond_function()
 ******************************************************************************
 * @brief
 *  This function is used to initialize the extent pool.
 *
 * @param base_object_p          - pointer to a base object.
 * @param packet_p               - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  06/06/2014 - Create. Lili Chen
 *
 ******************************************************************************/
static fbe_lifecycle_status_t 
fbe_extent_pool_init_pool_cond_function(fbe_base_object_t * base_object_p, fbe_packet_t * packet_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_extent_pool_t *     extent_pool_p = (fbe_extent_pool_t *)base_object_p;
    
    fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s \n", __FUNCTION__);

    status = fbe_extent_pool_init_disk_info(extent_pool_p);
    if (status != FBE_STATUS_OK) { 
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_extent_pool_initialize_pool_width(extent_pool_p);
    if (status != FBE_STATUS_OK) { 
        fbe_memory_release_required(extent_pool_p->disk_info_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_extent_pool_calculate_pool_blocks(extent_pool_p);
    if (status != FBE_STATUS_OK) { 
        fbe_memory_release_required(extent_pool_p->disk_info_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_extent_pool_init_hash_table(extent_pool_p);
    if (status != FBE_STATUS_OK) { 
        fbe_memory_release_required(extent_pool_p->disk_info_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_extent_pool_init_geometries(extent_pool_p);

    if (status != FBE_STATUS_OK) { 
        fbe_memory_release_required(extent_pool_p->hash_table);
        fbe_memory_release_required(extent_pool_p->disk_info_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* Fully map the entire pool. 
     * The pool will be one flat address space and each LUN will 
     * offset into the pool. 
     */
    status = fbe_extent_pool_fully_map_pool(extent_pool_p);
    if (status != FBE_STATUS_OK) { 
        fbe_memory_release_required(extent_pool_p->hash_table);
        fbe_memory_release_required(extent_pool_p->disk_info_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    status = fbe_extent_pool_construct_user_slices(extent_pool_p);
    if (status != FBE_STATUS_OK) { 
        fbe_memory_release_required(extent_pool_p->hash_table);
        fbe_memory_release_required(extent_pool_p->disk_info_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* clear the condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t *) extent_pool_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)extent_pool_p, 
                                FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X\n", __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/******************************************************************************
 * end fbe_extent_pool_init_pool_cond_function()
 ******************************************************************************/

/*!**************************************************************
 * fbe_extent_pool_attach_edge()
 ****************************************************************
 * @brief
 *  This function sets up the basic edge configuration info
 *  for this extent pool object.
 *  We also attempt to attach the edge.
 *
 * @param extent_pool_p - Pointer to extent pool object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/06/2014 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t
fbe_extent_pool_attach_edge(fbe_extent_pool_t * extent_pool_p, 
                            fbe_u32_t edge_index, 
                            fbe_object_id_t server_id, 
                            fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t width;	
    fbe_block_edge_t *edge_p = NULL;
    fbe_object_id_t my_object_id;
    fbe_block_transport_control_attach_edge_t attach_edge;
    fbe_package_id_t my_package_id;
    fbe_lba_t capacity;

    fbe_base_object_trace(  (fbe_base_object_t *)extent_pool_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Do some basic validation on the edge info received. */
    fbe_base_config_get_width((fbe_base_config_t *)extent_pool_p, &width);
    if (edge_index >= FBE_EXTENT_POOL_MAX_WIDTH) {
        fbe_base_object_trace(  (fbe_base_object_t *)extent_pool_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s edge index invalid %d\n", __FUNCTION__ , edge_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now attach the edge. */
    fbe_base_object_get_object_id((fbe_base_object_t *)extent_pool_p, &my_object_id);

    fbe_base_config_get_block_edge((fbe_base_config_t *)extent_pool_p, &edge_p, edge_index);
    fbe_block_transport_set_transport_id(edge_p);
    fbe_block_transport_set_server_id(edge_p, server_id);
    fbe_block_transport_set_client_id(edge_p, my_object_id);
    fbe_block_transport_set_client_index(edge_p, edge_index);
    /*!@todo server index needs to set with appropriate values if different client uses differnt server lba range. */
    fbe_block_transport_set_server_index(edge_p, 0);
    fbe_database_get_pvd_capacity(server_id, &capacity);
    fbe_block_transport_edge_set_capacity(edge_p, capacity);
    fbe_block_transport_edge_set_offset(edge_p, 0);
    
    /*first of all we need to check what is the lowest latency time on the edges above us, the lowest one wins*/
    //fbe_block_transport_edge_set_time_to_become_ready(edge_p, lowest_latency_time_in_seconds);

    attach_edge.block_edge = edge_p;

    fbe_get_package_id(&my_package_id);
    fbe_block_transport_edge_set_client_package_id(edge_p, my_package_id);

    status = fbe_base_config_send_attach_edge((fbe_base_config_t *)extent_pool_p, packet_p, &attach_edge) ;
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace(  (fbe_base_object_t *)extent_pool_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_base_config_attach_downstream_block_edge failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_extent_pool_attach_edge()
 ******************************************************************************/
