/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_bvd_interface_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the bvd interface object lifecycle code.
 * 
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_config_private.h"
#include "fbe_bvd_interface_private.h"

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t bvd_interface_detach_block_edges_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t bvd_interface_metadata_memory_init_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_status_t bvd_interface_metadata_memory_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

/***************************************************************************************************************************************/
fbe_status_t 
fbe_bvd_interface_monitor_entry(fbe_object_handle_t object_handle, 
                                fbe_packet_t * packet_p)
{
    fbe_status_t 				status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bvd_interface_t * 		bvd_p = NULL;

    bvd_p = (fbe_bvd_interface_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)bvd_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_lifecycle_crank_object(&fbe_bvd_interface_lifecycle_const, 
                                        (fbe_base_object_t*)bvd_p, packet_p);
										
    return status;
}

fbe_status_t fbe_bvd_interface_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(bvd_interface));
}


FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(bvd_interface);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(bvd_interface);

/*--- constant derived condition entries -----------------------------------------------*/
/* The BVD object would do it's own egde detach implementation */
static fbe_lifecycle_const_cond_t bvd_interface_detach_block_edges_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DETACH_BLOCK_EDGES,
        bvd_interface_detach_block_edges_cond_function)
};

/* BVD interface metadata memory initialization condition function. */
static fbe_lifecycle_const_cond_t bvd_interface_metadata_memory_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_METADATA_MEMORY_INIT,
        bvd_interface_metadata_memory_init_cond_function)
};


static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(bvd_interface) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        bvd_interface,
        FBE_LIFECYCLE_NULL_FUNC,          /* online function */
        FBE_LIFECYCLE_NULL_FUNC)   /* pending function */
};

FBE_LIFECYCLE_DEF_CONST_NULL_BASE_CONDITIONS(bvd_interface);

static fbe_lifecycle_const_rotary_cond_t bvd_interface_specialize_rotary[] = {
    /* Derived conditions */
   FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(bvd_interface_metadata_memory_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
};

static fbe_lifecycle_const_rotary_cond_t bvd_interface_destroy_rotary[] = {
    /* Derived conditions */
   FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(bvd_interface_detach_block_edges_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
};


static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(bvd_interface)[] = {
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, bvd_interface_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, bvd_interface_destroy_rotary),
};


FBE_LIFECYCLE_DEF_CONST_ROTARIES(bvd_interface);

/*--- global bvd_interface lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(bvd_interface) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        bvd_interface,
        FBE_CLASS_ID_BVD_INTERFACE,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_config))    /* The super class */
};



static fbe_lifecycle_status_t bvd_interface_detach_block_edges_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_status_t 				status = FBE_STATUS_GENERIC_FAILURE;
	fbe_bvd_interface_t * 		bvd_p = (fbe_bvd_interface_t *)object_p;

    fbe_base_object_lock(object_p);
    /* Let's check on the luns.  If still have lun(s) connected, let's wait for them to disconnect.
       If no lun is connected to us, clear the condition and move on*/ 
    if (fbe_queue_is_empty(&bvd_p->server_queue_head)) {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)bvd_p);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace(object_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't clear current condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
    }
    /* Still have lun(s) connected, this condition will ran until all luns are disconnected */
    fbe_base_object_unlock((fbe_base_object_t*)object_p);
    /* Return to the lifecycle */
    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
bvd_interface_metadata_memory_init_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_bvd_interface_t * 	bvd_p = (fbe_bvd_interface_t*)object_p;

    fbe_base_object_trace(object_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Set the completion function before we initialize the metadata memory. */
    fbe_transport_set_completion_function(packet_p, bvd_interface_metadata_memory_init_cond_completion, bvd_p);

    /* Initialize metadata memory for the provision drive object. */
    fbe_base_config_metadata_init_memory((fbe_base_config_t *) bvd_p, 
                                            packet_p,
                                            &bvd_p->bvd_metadata_memory,
											&bvd_p->bvd_metadata_memory_peer,
                                            sizeof(fbe_bvd_interface_metadata_memory_t));


    return FBE_LIFECYCLE_STATUS_PENDING;
}

static fbe_status_t bvd_interface_metadata_memory_init_cond_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
	fbe_bvd_interface_t * 				bvd_p = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    bvd_p = (fbe_bvd_interface_t*)context;

    /* Get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* Get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* Clear the metadata memory init condition if status is ok. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {    
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)bvd_p);
        if (status != FBE_STATUS_OK){
            fbe_base_object_trace((fbe_base_object_t*)bvd_p, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }

        fbe_topology_clear_gate(bvd_p->base_config.base_object.object_id);
    }

    /* If it returns busy status then do not clear the condition and try to 
     * initialize the metadata memory in next monitor cycle.
     */
    if(control_status == FBE_PAYLOAD_CONTROL_STATUS_BUSY) {
        fbe_base_object_trace((fbe_base_object_t*)bvd_p, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s can't clear current condition, payload status is FBE_PAYLOAD_CONTROL_STATUS_BUSY\n",
                                __FUNCTION__);
        
    }

    return FBE_STATUS_OK;

}

