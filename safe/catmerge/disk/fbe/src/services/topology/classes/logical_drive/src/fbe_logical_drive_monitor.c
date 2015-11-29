/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_drive_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the logical drive object lifecycle code.
 * 
 *  This includes the
 *  @ref fbe_logical_drive_monitor_entry "ldo's monitor entry point", as well as
 *  all the lifecycle defines such as rotaries and conditions, along with the
 *  actual condition functions.
 * 
 * @ingroup logical_drive_class_files
 * 
 * @version
 *   10/30/2007:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_logical_drive_private.h"
#include "fbe_logical_drive_io_command.h"
#include "fbe/fbe_enclosure.h"

/*!***************************************************************
 * fbe_logical_drive_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the logical drive's monitor.
 *
 * @param object_handle - This is the object handle, or in our case the ldo.
 * @param packet_p - The packet arriving from the monitor scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.             
 *
 * @author
 *  5/23/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_logical_drive_monitor_entry(fbe_object_handle_t object_handle, 
                                fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive_p = NULL;

    logical_drive_p = (fbe_logical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_lifecycle_crank_object(&fbe_logical_drive_lifecycle_const, 
                                        (fbe_base_object_t*)logical_drive_p, packet_p);
    return status;
}
/******************************************
 * end fbe_logical_drive_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_logical_drive_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the logical drive.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the logical drive's constant
 *                        lifecycle data.
 *
 * @author
 *  10/20/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_logical_drive_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(logical_drive));
}
/******************************************
 * end fbe_logical_drive_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t logical_drive_identity_unknown_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t logical_drive_identity_not_validated_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t logical_drive_block_edge_attached_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t logical_drive_block_edge_detached_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t logical_drive_block_edge_disabled_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t logical_drive_block_clients_connected_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_status_t           logical_drive_edge_detached_cond_completion1(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t fbe_logical_drive_pending_func(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t logical_drive_get_location_info_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_status_t           logical_drive_get_location_info_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t logical_drive_dequeue_pending_io_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t logical_drive_no_client_edges_connected_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t logical_drive_send_download_permit(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p);
static fbe_status_t           logical_drive_send_download_permit_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);


/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(logical_drive);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(logical_drive);

/*  logical_drive_lifecycle_callbacks
 *  This variable has all the callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(logical_drive) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        logical_drive,
        FBE_LIFECYCLE_NULL_FUNC,          /* online function */
        fbe_logical_drive_pending_func)   /* pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/

/*--- constant base condition entries --------------------------------------------------*/

/* logical_drive_block_edge_detached_cond 
 * FBE_LOGICAL_DRIVE_LIFECYCLE_COND_BLOCK_EDGE_DETACHED 
 * The purpose of this condition is to attach the block protocol edge.
 */
static fbe_lifecycle_const_base_cond_t logical_drive_block_edge_detached_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "logical_drive_block_edge_detached_cond",
        FBE_LOGICAL_DRIVE_LIFECYCLE_COND_BLOCK_EDGE_DETACHED,
        logical_drive_block_edge_detached_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* logical_drive_block_edge_disabled_cond
 * FBE_LOGICAL_DRIVE_LIFECYCLE_COND_BLOCK_EDGE_DISABLED
 * The purpose of this condition is to wait for the block edge to
 * enable.
 */
static fbe_lifecycle_const_base_cond_t logical_drive_block_edge_disabled_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "logical_drive_block_edge_disabled_cond",
        FBE_LOGICAL_DRIVE_LIFECYCLE_COND_BLOCK_EDGE_DISABLED,
        logical_drive_block_edge_disabled_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,             /* READY */
        FBE_LIFECYCLE_STATE_ACTIVATE,             /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* logical_drive_identity_unknown_cond
 * FBE_LOGICAL_DRIVE_LIFECYCLE_COND_IDENTITY_UNKNOWN 
 * The purpose of this condition is to fetch the identity and store it for later
 * use. 
 */
static fbe_lifecycle_const_base_cond_t logical_drive_identity_unknown_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "logical_drive_identity_unknown_cond",
        FBE_LOGICAL_DRIVE_LIFECYCLE_COND_IDENTITY_UNKNOWN,
        logical_drive_identity_unknown_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* logical_drive_identity_not_validated_cond 
 * FBE_LOGICAL_DRIVE_LIFECYCLE_COND_IDENTITY_NOT_VALIDATED 
 *  The purpose of this condition is to validate the identity by fetching it
 *  and comparing it with the value stored at specialze time.
 */
static fbe_lifecycle_const_base_cond_t logical_drive_identity_not_validated_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "logical_drive_identity_not_validated",
        FBE_LOGICAL_DRIVE_LIFECYCLE_COND_IDENTITY_NOT_VALIDATED,
        logical_drive_identity_not_validated_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* logical_drive_block_edge_attached_cond
 *   FBE_LOGICAL_DRIVE_LIFECYCLE_COND_BLOCK_EDGE_ATTACHED
 *   The purpose of this condition is to detach the block transport edge.
 */ 
static fbe_lifecycle_const_base_cond_t logical_drive_block_edge_attached_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "logical_drive_block_edge_attached_cond",
        FBE_LOGICAL_DRIVE_LIFECYCLE_COND_BLOCK_EDGE_ATTACHED,
        logical_drive_block_edge_attached_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* logical_drive_clients_connected_cond
 * FBE_LOGICAL_DRIVE_LIFECYCLE_COND_CLIENTS_CONNECTED
 *  The purpose of this condition is to mark the client edges gone and
 *  wait for the clients to disconnect.
 */ 
static fbe_lifecycle_const_base_cond_t logical_drive_clients_connected_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "logical_drive_clients_connected_cond",
        FBE_LOGICAL_DRIVE_LIFECYCLE_COND_CLIENTS_CONNECTED,
        logical_drive_block_clients_connected_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/*
 *  The purpose of this condition is to obtain location information
 *  such as enclosure number and slot number
 */ 
static fbe_lifecycle_const_base_cond_t logical_drive_get_location_info_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "logical_drive_get_location_info_cond",
        FBE_LOGICAL_DRIVE_LIFECYCLE_COND_GET_LOCATION_INFO,
        logical_drive_get_location_info_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


static fbe_lifecycle_const_base_cond_t logical_drive_dequeue_pending_io_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "logical_drive_dequeue_pending_io_cond",
        FBE_LOGICAL_DRIVE_LIFECYCLE_COND_DEQUEUE_PENDING_IO,
        logical_drive_dequeue_pending_io_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,             /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t logical_drive_no_client_edges_connected_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "logical_drive_no_client_edges_connected_cond",
        FBE_LOGICAL_DRIVE_LIFECYCLE_COND_NO_CLIENT_EDGES_CONNECTED,
        logical_drive_no_client_edges_connected_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,              /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};



/* logical_drive_lifecycle_base_cond_array
 * This is our static list of base conditions.
 */
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(logical_drive)[] = {
    &logical_drive_identity_unknown_cond,
    &logical_drive_block_edge_detached_cond,
    &logical_drive_block_edge_disabled_cond,
    &logical_drive_identity_not_validated_cond,
    &logical_drive_block_edge_attached_cond,
    &logical_drive_clients_connected_cond,
    &logical_drive_get_location_info_cond,
    &logical_drive_dequeue_pending_io_cond,
    &logical_drive_no_client_edges_connected_cond
};

/* logical_drive_lifecycle_base_conditions 
 *  This is the list of base default base conditions for the logical
 *  drive.
 */
FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(logical_drive);


/*--- constant rotaries ----------------------------------------------------------------*/
static fbe_lifecycle_const_rotary_cond_t logical_drive_specialize_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(logical_drive_block_edge_detached_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(logical_drive_block_edge_disabled_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(logical_drive_identity_unknown_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t logical_drive_activate_rotary[] = {
    /* Derived conditions */

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(logical_drive_identity_not_validated_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(logical_drive_get_location_info_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t logical_drive_destroy_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(logical_drive_clients_connected_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(logical_drive_block_edge_attached_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t logical_drive_ready_rotary[] = {
    /* Derived conditions */
    
    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(logical_drive_dequeue_pending_io_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(logical_drive_no_client_edges_connected_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};




static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(logical_drive)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, logical_drive_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, logical_drive_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, logical_drive_destroy_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, logical_drive_ready_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(logical_drive);

/*--- global logical_drive lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(logical_drive) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        logical_drive,
        FBE_CLASS_ID_LOGICAL_DRIVE,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_discovered))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/

/*--- Condition Functions --------------------------------------------------------------*/

/*!**************************************************************
 * logical_drive_identity_unknown_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function where we
 *  try to obtain our identity for the first time.
 *
 * @param object_p - The pointer to the logical drive.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Always FBE_LIFECYCLE_STATUS_PENDING.
 *
 * @author
 *  10/20/2008 - Created. RPF
 *
 ****************************************************************/

static fbe_lifecycle_status_t 
logical_drive_identity_unknown_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive_p = (fbe_logical_drive_t*)object_p;

    fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack.
     */
    status = fbe_transport_set_completion_function(packet_p , logical_drive_identity_unknown_cond_completion,  logical_drive_p);

    /* Call the usurper function to do actual job.
     */
    fbe_ldo_get_identity(logical_drive_p, packet_p );

    /* Return pending to the lifecycle.
     */
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************
 * end logical_drive_identity_unknown_cond_function()
 ******************************************/

/*!**************************************************************
 * logical_drive_identity_unknown_cond_completion()
 ****************************************************************
 * @brief
 *  This is the completion of the condition function where we
 *  try to obtain our identity for the first time.
 * 
 *  When we enter this function, the identify has just completed.
 *  We will save the results of the identify.
 *
 * @param packet_p - The packet sent to us from the scheduler.
 * @param context - The ptr to the logical drive.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  10/20/2008 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
logical_drive_identity_unknown_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive_p = NULL;
    
    logical_drive_p = (fbe_logical_drive_t*)context;

    fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Only clear the condition if the packet completion status is OK.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status == FBE_STATUS_OK)
    {
        /* On success, we will save the identity information.
         */
        fbe_copy_memory(&logical_drive_p->identity,
                        &logical_drive_p->last_identity,
                        sizeof(fbe_block_transport_identify_t));

        status = fbe_ldo_clear_condition((fbe_base_object_t*)logical_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%X",
                                  __FUNCTION__, status);
        }
    } /* end status is OK */
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s get identity failed, status: 0x%X",
                              __FUNCTION__, status);
    }  /* end status is not OK */

    /* Returning OK in this case causes the packet which we originally issued 
     * (the monitor packet), to get completed.  Thus, there is no need to 
     * complete the monitor packet itself.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end logical_drive_identity_unknown_cond_completion()
 ******************************************/

/*!**************************************************************
 * logical_drive_identity_unknown_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function where we will attempt to validate the
 *  identity information.
 *  We assume that we previously executed the identity unknown
 *  condition and saved the identity inside the logical drive.
 * 
 *  Now we will perform an identify again and compare the results
 *  to the saved results in the logical drive.
 *
 * @param object_p - The pointer to the logical drive.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Always FBE_LIFECYCLE_STATUS_PENDING.
 *
 * @author
 *  10/20/2008 - Created. RPF
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
logical_drive_identity_not_validated_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive_p = (fbe_logical_drive_t*)object_p;

    fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack.
     */
    status = fbe_transport_set_completion_function(packet_p, 
                                                   logical_drive_identity_not_validated_cond_completion, 
                                                   logical_drive_p);
    /* Call the usurper function to do actual job.
     */
    fbe_ldo_get_identity(logical_drive_p, packet_p );

    /* Return pending to the lifecycle.
     */
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************
 * end logical_drive_identity_not_validated_cond_function()
 ******************************************/

/*!**************************************************************
 * logical_drive_identity_not_validated_cond_completion()
 ****************************************************************
 * @brief
 *  This is the completion of the condition function where we
 *  are validating that our identity information has not changed.
 * 
 *  The identify has completed and we will compare the
 *  results of the identify with the results already stored
 *  inside the logical drive.
 *
 * @param packet_p - The packet sent to us from the scheduler.
 * @param context - The pointer to the logical drive.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  10/20/2008 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
logical_drive_identity_not_validated_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive_p = NULL;
    
    logical_drive_p = (fbe_logical_drive_t*)context;

    fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Only clear the condition if the packet completion status is OK.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status == FBE_STATUS_OK)
    {
        /* Compare the retrieved identity to the identity that we saved.
         */ 
        fbe_u32_t index;
        fbe_bool_t b_match = FBE_TRUE;
        for(index = 0; index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH; index++)
        {
            if (logical_drive_p->identity.identify_info[index] !=  
                logical_drive_p->last_identity.identify_info[index])
            {
                b_match = FBE_FALSE;
            }
        }

        if (b_match == FBE_FALSE)
        {
            /* There was a mismatch between the latest identity and 
             * the identity that we received. 
             * Transition to destroy. 
             */
            fbe_ldo_set_condition(&fbe_logical_drive_lifecycle_const, 
                                  (fbe_base_object_t*)logical_drive_p,
                                  FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
            return FBE_STATUS_OK;
        }

        status = fbe_ldo_clear_condition((fbe_base_object_t*)logical_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%X",
                                  __FUNCTION__, status);
        }
    } /* end status is OK. */
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s get identity failed, status: 0x%X",
                              __FUNCTION__, status);
    }  /* end status is not OK */

    /* Returning OK in this case causes the packet which we originally issued 
     * (the monitor packet), to get completed.  Thus, there is no need to 
     * complete the monitor packet itself.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end logical_drive_identity_not_validated_cond_completion()
 ******************************************/

/*!**************************************************************
 * logical_drive_block_edge_detached_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function where we are trying to
 *  attach the block edge to the physical drive.
 * 
 *  We will first try to get the object id from the physical drive.
 *
 * @param object_p - The pointer to the logical drive.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Always FBE_LIFECYCLE_STATUS_PENDING.
 *
 * @author
 *  10/20/2008 - Created. RPF
 *
 ****************************************************************/
static fbe_lifecycle_status_t
logical_drive_block_edge_detached_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive = (fbe_logical_drive_t*)object_p;

    fbe_base_object_trace((fbe_base_object_t*)logical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack.
     */
    status = fbe_transport_set_completion_function(packet_p , logical_drive_block_edge_detached_cond_completion, logical_drive);

    /* Get the object id from the physical drive and attach the block edge.
     * Call the usurper function. 
     */
    fbe_ldo_send_get_object_id_command(logical_drive, packet_p );

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************
 * end logical_drive_block_edge_detached_cond_function()
 ******************************************/

/*!**************************************************************
 * logical_drive_block_edge_detached_cond_completion()
 ****************************************************************
 * @brief
 *  This is the condition function completion where we are trying to
 *  attach our edge.
 *
 * @param packet_p - The packet sent to us from the scheduler.
 * @param context - The pointer to the logical drive.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  10/20/2008 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
logical_drive_block_edge_detached_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive_p = NULL;
    
    logical_drive_p = (fbe_logical_drive_t*)context;

    /* Only clear the condition if the packet completion status is OK.
     */
    status = fbe_transport_get_status_code(packet_p);

    if (status == FBE_STATUS_OK)
    {
        status = fbe_ldo_clear_condition((fbe_base_object_t*)logical_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%X",
                                  __FUNCTION__, status);
        }
    } /* end if status is OK*/
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s attach failed, status: 0x%X\n",
                              __FUNCTION__, status);
    } /* end status is not OK */

    /* Returning OK in this case causes the packet which we originally issued 
     * (the monitor packet), to get completed.  Thus, there is no need to 
     * complete the monitor packet itself.
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end logical_drive_block_edge_detached_cond_completion()
 ******************************************/

/*!**************************************************************
 * logical_drive_block_edge_disabled_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function where we are trying to get
 *  our edge to come enabled.
 *
 * @param object_p - The ptr to the base object of the logical drive.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Always FBE_LIFECYCLE_STATUS_PENDING.
 *
 * @author
 *  10/20/2008 - Created. RPF
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
logical_drive_block_edge_disabled_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive = (fbe_logical_drive_t*)object_p;

    fbe_base_object_trace((fbe_base_object_t*)logical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet_p , logical_drive_block_edge_disabled_cond_completion, logical_drive);

    /* Call the usurper function to do actual job */
    fbe_ldo_open_block_edge(logical_drive, packet_p );

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************
 * end logical_drive_block_edge_disabled_cond_function()
 ******************************************/

/*!**************************************************************
 * logical_drive_block_edge_disabled_cond_completion()
 ****************************************************************
 * @brief
 *  This is the condition function completion where we
 *  are trying to get our edge to come enabled.
 *
 * @param packet_p - The packet sent to us from the scheduler.
 * @param context - The pointer to the logical drive.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  10/20/2008 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
logical_drive_block_edge_disabled_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive_p = NULL;
    
    logical_drive_p = (fbe_logical_drive_t*)context;

    /* Only clear the condition if the packet completion status is OK.
     */
    status = fbe_transport_get_status_code(packet_p);

    if (status == FBE_STATUS_OK)
    {
        status = fbe_ldo_clear_condition((fbe_base_object_t*)logical_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%X",
                                  __FUNCTION__, status);
        }
    }  /* end if status is OK */
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s edge enable failed, status: 0x%X",
                              __FUNCTION__, status);
    } /* end status is not OK */
    return FBE_STATUS_OK;
}
/******************************************
 * end logical_drive_block_edge_disabled_cond_completion()
 ******************************************/

/*!**************************************************************
 * logical_drive_block_edge_attached_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function where we are trying to
 *  disconnect our edge from the physical drive.
 *
 * @param object_p - The ptr to the base object of the logical drive.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Always FBE_LIFECYCLE_STATUS_PENDING.
 *
 * @author
 *  10/20/2008 - Created. RPF
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
logical_drive_block_edge_attached_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive_p = (fbe_logical_drive_t *)object_p;

    fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet_p, 
                                                   logical_drive_block_edge_attached_cond_completion, 
                                                   logical_drive_p);

    fbe_ldo_detach_block_edge(logical_drive_p, packet_p);
    
    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************
 * end logical_drive_block_edge_attached_cond_function()
 ******************************************/

/*!**************************************************************
 * logical_drive_block_edge_attached_cond_completion()
 ****************************************************************
 * @brief
 *  This is the completion for the condition function
 *  where we are trying to disconnect our edge from the physical drive.
 *
 * @param packet_p - The packet sent to us from the scheduler.
 * @param context - The pointer to the logical drive.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  10/20/2008 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
logical_drive_block_edge_attached_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive_p = NULL;
    
    logical_drive_p = (fbe_logical_drive_t *)context;

    fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Only clear the condition if the packet completion status is OK.
     */
    status = fbe_transport_get_status_code(packet_p);

    if (status == FBE_STATUS_OK)
    {
        status = fbe_ldo_clear_condition((fbe_base_object_t*)logical_drive_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%X",
                                  __FUNCTION__, status);
        }
    }    /* end if status is OK*/
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s edge detach failed, status: 0x%X",
                              __FUNCTION__, status);
    } /* end detach failed. */
    return FBE_STATUS_OK;
}
/******************************************
 * end logical_drive_block_edge_attached_cond_completion()
 ******************************************/

/*!**************************************************************
 * logical_drive_block_clients_connected_cond_function()
 ****************************************************************
 * @brief
 *  This is the condition function where we are waiting for
 *  clients to disconnect their edges.
 *
 * @param object_p - The ptr to the base object of the logical drive.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Always FBE_LIFECYCLE_STATUS_PENDING.
 *
 * @author
 *  10/20/2008 - Created. RPF
 *
 ****************************************************************/
fbe_lifecycle_status_t
logical_drive_block_clients_connected_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive_p = (fbe_logical_drive_t *)object_p;
    fbe_u32_t number_of_clients;
    fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* First, check if we have any client edges.
     */
    status = fbe_block_transport_server_get_number_of_clients(&logical_drive_p->block_transport_server,
                                                              &number_of_clients);

    if (status == FBE_STATUS_OK && number_of_clients == 0)
    {
        /* If there are no attached clients, then it is OK to clear the condition.
         */ 
        status = fbe_ldo_clear_condition((fbe_base_object_t*)logical_drive_p);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%X",
                                  __FUNCTION__, status);
        }
        else
        {
            /* Sucessfully cleared the condition, we are done. 
             */ 
            return FBE_LIFECYCLE_STATUS_DONE;
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s clients attached 0x%x %d\n", __FUNCTION__, status, number_of_clients);
    }
    /* Return pending to the lifecycle.
     */
    return FBE_LIFECYCLE_STATUS_PENDING;
}
/******************************************
 * end logical_drive_block_clients_connected_cond_function()
 ******************************************/

/*!**************************************************************
 * fbe_logical_drive_pending_func()
 ****************************************************************
 * @brief
 *   This gets called when we are going to transition from one
 *   lifecycle state to another. The purpose here is to drain
 *   all I/O before we transition state.
 *  
 * @param base_object - Object that is pending.
 * @param packet - The monitor paacket.
 * 
 * @return fbe_lifecycle_status_t - We return the status
 *         from fbe_block_transport_server_pending() since
 *         that is the function which deals with determining
 *         when I/Os are drained.
 *
 * @author
 *  1/16/2009 - Created. RPF
 *
 ****************************************************************/

static fbe_lifecycle_status_t fbe_logical_drive_pending_func(fbe_base_object_t * base_object, 
                                                             fbe_packet_t * packet)
{
    fbe_lifecycle_status_t lifecycle_status;
    fbe_logical_drive_t * logical_drive_p = NULL;

    logical_drive_p = (fbe_logical_drive_t *)base_object;

    fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry\n", __FUNCTION__);

    /* We simply call to the block transport server to handle pending.
     */
    lifecycle_status = fbe_block_transport_server_pending(&logical_drive_p->block_transport_server,
                                                          &fbe_logical_drive_lifecycle_const,
                                                          (fbe_base_object_t *) logical_drive_p);

    return lifecycle_status;
}
/**************************************
 * end fbe_logical_drive_pending_func()
 **************************************/

static fbe_lifecycle_status_t 
logical_drive_get_location_info_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive_p = (fbe_logical_drive_t *)object_p;

    fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    if(logical_drive_p->port_number != FBE_PORT_NUMBER_INVALID &&
        logical_drive_p->enclosure_number != FBE_ENCLOSURE_NUMBER_INVALID &&
        logical_drive_p->slot_number != FBE_SLOT_NUMBER_INVALID){

        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)logical_drive_p);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet_p, 
                                                   logical_drive_get_location_info_cond_completion, 
                                                   logical_drive_p);

    fbe_logical_drive_get_location_info(logical_drive_p, packet_p);
    
    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t 
logical_drive_get_location_info_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_logical_drive_t * logical_drive_p = NULL;
    
    logical_drive_p = (fbe_logical_drive_t*)context;

    fbe_base_object_trace((fbe_base_object_t*)logical_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Only clear the condition if the packet completion status is OK.
     */
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s get identity failed, status: 0x%X",
                              __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }  /* end status is not OK */


    if(logical_drive_p->port_number != FBE_PORT_NUMBER_INVALID &&
        logical_drive_p->enclosure_number != FBE_ENCLOSURE_NUMBER_INVALID &&
        logical_drive_p->slot_number != FBE_SLOT_NUMBER_INVALID){

        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)logical_drive_p);
    }

    /* Returning OK in this case causes the packet which we originally issued 
     * (the monitor packet), to get completed.  Thus, there is no need to 
     * complete the monitor packet itself.
     */
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t logical_drive_dequeue_pending_io_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_logical_drive_t *   logical_drive_p = (fbe_logical_drive_t*)base_object;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_lifecycle_clear_current_cond(base_object);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    /*need to send all IOs that were queud for some reason(e.g. we were hibernating and we got IO)*/
    fbe_block_transport_server_process_io_from_queue(&logical_drive_p->block_transport_server);

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * logical_drive_no_client_edges_connected_cond_function()
 ****************************************************************
 * @brief
 *   This is a condition function that does special handling for
 *   cases where an edge attribute changed and can't be propagated
 *   because no client edges are connected.
 *  
 * @param base_object - Object that is pending.
 * @param packet - The monitor paacket.
 * 
 * @return fbe_lifecycle_status_t
 *
 * @author
 *  12/22/2012 - Wayne Garrett - Created.
 *
 ****************************************************************/
static fbe_lifecycle_status_t logical_drive_no_client_edges_connected_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_logical_drive_t *logical_drive_p = (fbe_logical_drive_t*)base_object;
    fbe_path_attr_t path_attr = 0;
    fbe_status_t status;

    fbe_block_transport_get_path_attributes(&logical_drive_p->block_edge, &path_attr);

    /* if download request sent and no SEP edges to propagate to, then it's safe to
       issue the download */
    if (path_attr & (FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ |
                     FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_FAST_DL |
                     FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_TRIAL_RUN))
    {
        return logical_drive_send_download_permit(logical_drive_p, packet);
    }

    status = fbe_lifecycle_clear_current_cond(base_object);
        
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!****************************************************************************
 * logical_drive_send_download_permit()
 ******************************************************************************
 * @brief
 *  This function sends download permit to PDO. 
 *
 * @param logical_drive_p            - pointer to a logical drive.
 * @param packet_p                   - pointer to a control packet.
 * 
 * @return fbe_lifecycle_status_t 
 *
 * @author
 *  12/22/2012 - Wayne Garrett - Created. 
 *
 ******************************************************************************/
static fbe_lifecycle_status_t logical_drive_send_download_permit(fbe_logical_drive_t * logical_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(control_operation_p, 0);

    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_PERMIT,
                                        NULL,
                                        0);

    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
    fbe_transport_set_completion_function(packet_p, logical_drive_send_download_permit_completion, logical_drive_p);

    fbe_block_transport_send_control_packet(&logical_drive_p->block_edge, packet_p); 
    
    return FBE_LIFECYCLE_STATUS_PENDING;
}

/*!****************************************************************************
 * logical_drive_send_download_permit_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for sending PDO usurper commands
 * 
 * @param packet_p   - Pointer to the packet.
 * @param context    - context, which is a pointer to the base object.
 *
 * @return fbe_status_t
 *
 * @author
 *  02/01/2013  Wayne Garrett - Created.
 * 
 ******************************************************************************/
static fbe_status_t logical_drive_send_download_permit_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_logical_drive_t *               logical_drive_p = (fbe_logical_drive_t*)context;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_status_t        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Get the control operation status and packet status */
    fbe_payload_control_get_status(control_operation_p, &control_status);
    status = fbe_transport_get_status_code(packet_p);

    if (status != FBE_STATUS_OK || control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)logical_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed pkt_status:%d control_status:%d\n", 
                                __FUNCTION__, status, control_status);
    }

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)logical_drive_p);
        
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)logical_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s can't clear current condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }

    /* Release the control operation */
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

    return FBE_STATUS_OK;
}

/*******************************
 * end fbe_logical_drive_monitor.c
 *******************************/
