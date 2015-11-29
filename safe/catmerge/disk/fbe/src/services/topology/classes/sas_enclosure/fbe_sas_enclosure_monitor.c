/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_sas_enclosure_monitor.c
 ***************************************************************************
 *
 *  @brief
 *  This file contains condition handler for SAS enclosure class.
 *
 * @ingroup fbe_enclosure_class
 * 
 * HISTORY
 *   11/11/2008:  Created file header - NC
 *   11/11/2008:  Added SMP edge - NC
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_sas_enclosure.h"
#include "fbe_scheduler.h"
#include "fbe_transport_memory.h"

#include "fbe_base_discovered.h"
#include "base_enclosure_private.h"
#include "sas_enclosure_private.h"

/*!*************************************************************************
* @fn fbe_sas_enclosure_monitor_entry(                  
*                    fbe_object_handle_t object_handle, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This is the entry point for the monitor.
*
* @param      object_handle - object handle.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_status_t.
*
* NOTES
*
* HISTORY
*   19-Nov-2008 NC - Added header.
*
***************************************************************************/
fbe_status_t 
fbe_sas_enclosure_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = NULL;

    sas_enclosure = (fbe_sas_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                        "%s entry\n", __FUNCTION__);

    status = fbe_lifecycle_crank_object(&fbe_sas_enclosure_lifecycle_const, (fbe_base_object_t*)sas_enclosure, packet);

    return status;  
}

fbe_status_t fbe_sas_enclosure_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(sas_enclosure));
}

/*--- local function prototypes --------------------------------------------------------*/

/* This is a no-op condition function that just transitions the lifecycle state, Multiple derived conditions call this. */
static fbe_lifecycle_status_t sas_enclosure_noop_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t sas_enclosure_get_smp_address_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_status_t sas_enclosure_get_smp_address_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_enclosure_open_smp_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_enclosure_open_smp_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_enclosure_get_virtual_port_address_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_status_t sas_enclosure_get_virtual_port_address_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_enclosure_attach_smp_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_enclosure_attach_ssp_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t sas_enclosure_open_ssp_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_enclosure_open_ssp_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_enclosure_inquiry_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_enclosure_inquiry_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t sas_enclosure_get_element_list_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_status_t sas_enclosure_get_element_list_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);


static fbe_lifecycle_status_t sas_enclosure_detach_smp_edge_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_enclosure_detach_ssp_edge_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t sas_enclosure_common_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);



/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(sas_enclosure);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(sas_enclosure);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(sas_enclosure) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        sas_enclosure,
        FBE_LIFECYCLE_NULL_FUNC,       /* online function */
        FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/

static fbe_lifecycle_const_cond_t sas_enclosure_discovery_poll_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_TIMER_COND(
        FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL,
        sas_enclosure_noop_cond_function)
};

static fbe_lifecycle_const_cond_t sas_enclosure_attach_ssp_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_ATTACH_PROTOCOL_EDGE,
        sas_enclosure_attach_ssp_edge_cond_function)
};

static fbe_lifecycle_const_cond_t sas_enclosure_open_protocol_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_DISCOVERED_LIFECYCLE_COND_OPEN_PROTOCOL_EDGE,
        sas_enclosure_noop_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/
/* FBE_SAS_ENCLOSURE_LIFECYCLE_COND_INQUIRY condition */
static fbe_lifecycle_const_base_cond_t sas_enclosure_inquiry_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_enclosure_inquiry_cond",
        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_INQUIRY,
        sas_enclosure_inquiry_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_ELEMENT_LIST condition */
static fbe_lifecycle_const_base_cond_t sas_enclosure_get_element_list_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_enclosure_get_element_list_cond",
        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_ELEMENT_LIST,
        sas_enclosure_get_element_list_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_SAS_ENCLOSURE_LIFECYCLE_COND_ATTACH_SMP_EDGE condition 
    This condition is preset in destroy rotary
*/
static fbe_lifecycle_const_base_cond_t sas_enclosure_attach_smp_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_enclosure_attach_smp_edge_cond",
        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_ATTACH_SMP_EDGE,
        sas_enclosure_attach_smp_edge_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_SMP_ADDRESS condition */
static fbe_lifecycle_const_base_cond_t sas_enclosure_get_smp_address_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_enclosure_get_smp_address_cond",
        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_SMP_ADDRESS,
        sas_enclosure_get_smp_address_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_SAS_ENCLOSURE_LIFECYCLE_COND_OPEN_SMP_EDGE condition */
static fbe_lifecycle_const_base_cond_t sas_enclosure_open_smp_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_enclosure_open_smp_edge_cond",
        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_OPEN_SMP_EDGE,
        sas_enclosure_open_smp_edge_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_VIRTUAL_PORT_ADDRESS condition */
static fbe_lifecycle_const_base_cond_t sas_enclosure_get_virtual_port_address_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_enclosure_get_virtual_port_address_cond",
        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_VIRTUAL_PORT_ADDRESS,
        sas_enclosure_get_virtual_port_address_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,               /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* FBE_SAS_ENCLOSURE_LIFECYCLE_COND_OPEN_SSP_EDGE condition */
static fbe_lifecycle_const_base_cond_t sas_enclosure_open_ssp_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_enclosure_open_ssp_edge_cond",
        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_OPEN_SSP_EDGE,
        sas_enclosure_open_ssp_edge_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,          /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,            /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,           /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,             /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,                /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};


/* FBE_SAS_ENCLOSURE_LIFECYCLE_COND_DETACH_SSP_EDGE condition 
    This condition is preset in destroy rotary
*/
static fbe_lifecycle_const_base_cond_t sas_enclosure_detach_ssp_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_enclosure_detach_ssp_edge_cond",
        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_DETACH_SSP_EDGE,
        sas_enclosure_detach_ssp_edge_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* FBE_SAS_ENCLOSURE_LIFECYCLE_COND_DETACH_SMP_EDGE condition 
    This condition is preset in destroy rotary
*/
static fbe_lifecycle_const_base_cond_t sas_enclosure_detach_smp_edge_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_enclosure_detach_smp_edge_cond",
        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_DETACH_SMP_EDGE,
        sas_enclosure_detach_smp_edge_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

/* 
 * The conditions here in the BASE_COND_ARRY must match the condition order 
 * in the fbe_sas_enclosure_lifecycle_cond_id_e enum.
 */
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(sas_enclosure)[] = {    
    &sas_enclosure_attach_smp_edge_cond,
    &sas_enclosure_get_smp_address_cond,
    &sas_enclosure_open_smp_edge_cond,
    &sas_enclosure_get_virtual_port_address_cond,
    &sas_enclosure_open_ssp_edge_cond,
    &sas_enclosure_inquiry_cond,    
    &sas_enclosure_get_element_list_cond,
    &sas_enclosure_detach_ssp_edge_cond,
    &sas_enclosure_detach_smp_edge_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(sas_enclosure);


/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t sas_enclosure_specialize_rotary[] = {
    /* Derived conditions will be run in their order from the derived rotary. */
    /* FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_get_protocol_address_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET), */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_attach_ssp_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_open_protocol_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),    
    
    /* Base conditions will be run in the order shown here. */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_attach_smp_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_get_smp_address_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_open_smp_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_get_virtual_port_address_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_open_ssp_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_inquiry_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

/*activate rotary*/
static fbe_lifecycle_const_rotary_cond_t sas_enclosure_activate_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_get_smp_address_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_open_smp_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_get_virtual_port_address_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_open_ssp_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t sas_enclosure_ready_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_discovery_poll_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_get_element_list_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t sas_enclosure_destroy_rotary[] = {
    /* Derived conditions */

    /* Base conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_detach_ssp_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_enclosure_detach_smp_edge_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};


static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(sas_enclosure)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, sas_enclosure_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, sas_enclosure_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, sas_enclosure_ready_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, sas_enclosure_destroy_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(sas_enclosure);

/*--- global sas enclosure lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_enclosure) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        sas_enclosure,
        FBE_CLASS_ID_SAS_ENCLOSURE,                  /* This class */
        FBE_LIFECYCLE_CONST_DATA(base_enclosure))    /* The super class */
};


/*--- Condition Functions --------------------------------------------------------------*/


/*!***************************************************************
 * @fn sas_enclosure_noop_cond_function(
 *                    fbe_base_object_t * p_object, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This condition function simply transitions the rotary to the
 *  next condition.  It is used to make a derived condition a no-op.
 *
 * @param p_object - our object context
 * @param packet - request packet
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  7/28/08 - Created. bphilbin
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_enclosure_noop_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = (fbe_sas_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                        "%s entry\n", __FUNCTION__);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                            "%s can't clear current condition, status: 0x%X\n", 
                            __FUNCTION__, status);
       
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}


/*!***************************************************************
 * @fn sas_enclosure_get_smp_address_cond_function(
 *                    fbe_base_object_t * p_object, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This condition function notifies the executer that we need to 
 *  issue a request to the port object to obtain the SMP address 
 *  (address of the physical "primary port" connection).
 *
 * @param p_object - our object context
 * @param packet - request packet
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  7/28/08 - Created. bphilbin
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_enclosure_get_smp_address_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = (fbe_sas_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                        "%s entry\n", __FUNCTION__);

    /* Initialize the SMP (physical) address. */
    sas_enclosure->sasEnclosureSMPAddress = FBE_SAS_ADDRESS_INVALID;

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_enclosure_get_smp_address_cond_completion, sas_enclosure);

    /* Call the executer function to do actual job */
    fbe_sas_enclosure_get_smp_address(sas_enclosure, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}/* end sas_enclosure_get_smp_address_cond_function */

/*!***************************************************************
 * @fn sas_enclosure_get_smp_address_cond_completion(
 *                    fbe_packet_t * packet, 
 *                    fbe_packet_completion_context_t context)                  
 ****************************************************************
 * @brief
 *  This condition function handles the response to the request
 *  to the port object to obtain the SMP address 
 *  (address of the physical "primary port" connection).
 *
 * @param  packet - request packet
 * @param  context - what it says
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/25/08 - Created. NC
 *
 ****************************************************************/
static fbe_status_t 
sas_enclosure_get_smp_address_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = NULL;
    fbe_u32_t retry_count = 0;    
    
    sas_enclosure = (fbe_sas_enclosure_t*)context;

    /* Check packet status and verify that sasEnclosureSMPAddress was found*/
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_WARNING,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s Invalid packet status: 0x%X\n",
                                        __FUNCTION__, status);

        return FBE_STATUS_OK; /* this is completion OK */
    }

    if(sas_enclosure->sasEnclosureSMPAddress == FBE_SAS_ADDRESS_INVALID) {
        retry_count = fbe_base_enclosure_increment_condition_retry_count((fbe_base_enclosure_t*)sas_enclosure);
        if ((retry_count%SAS_ENCLOSURE_LOG_SUPPRESS_COUNT) == 1)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s No address: 0x%llX\n",
                                            __FUNCTION__, (unsigned long long)sas_enclosure->sasEnclosureSMPAddress);
        }
        /*
         * If we failed to get the virtual port address, check if we need to 
         * enter the reset ride through.
         */         
        fbe_sas_enclosure_check_reset_ride_through_needed((fbe_base_object_t*)sas_enclosure);
        
        
        return FBE_STATUS_OK; /* this is completion OK */
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_DEBUG_LOW,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s Got a valid SMP address: 0x%llX\n",
                                            __FUNCTION__, (unsigned long long)sas_enclosure->sasEnclosureSMPAddress);
    }

    /*
     * We have good status and got an address for our
     * virtual port.  It is safe to Clear the condition now. 
     */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s can't clear current condition, status: 0x%X\n",
                                            __FUNCTION__, status);
        
    }

    return FBE_STATUS_OK;

   

}/* end sas_enclosure_get_smp_address_cond_completion */

/*!***************************************************************
 * @fn sas_enclosure_get_virtual_port_address_cond_function(
 *                    fbe_base_object_t * p_object, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This condition function notifies the executer that we need to 
 *  issue a request to the enclosure to via a get element list
 *  request to obtain the address of the virtual/SES port.
 *
 * @param  p_object - our object context
 * @param  packet - request packet
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  7/28/08 - Created. bphilbin
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_enclosure_get_virtual_port_address_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = (fbe_sas_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry \n", __FUNCTION__);


    /* Initialize the virtual port address. */
    sas_enclosure->ses_port_address = FBE_SAS_ADDRESS_INVALID;

     /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_enclosure_get_virtual_port_address_cond_completion, sas_enclosure);

    /* Call the executer function to do actual job */
    status = fbe_sas_enclosure_send_get_element_list_command(sas_enclosure, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}/* end sas_enclosure_get_virtual_port_address_cond_function */


/*!***************************************************************
 * @fn sas_enclosure_get_virtual_port_address_cond_completion(
 *                    fbe_packet_t * packet, 
 *                    fbe_packet_completion_context_t context)                  
 ****************************************************************
 * @brief
 *  This condition function handles the response to the request
 *  to the port object to obtain the virtual port address. 
 *
 * @param  packet - request packet
 * @param  context - what it says
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  7/28/08 - Created. bphilbin
 *
 ****************************************************************/
static fbe_status_t 
sas_enclosure_get_virtual_port_address_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = NULL;
    fbe_u32_t retry_count = 0;
    
    sas_enclosure = (fbe_sas_enclosure_t*)context;

    /* Check packet status and verify that a virtual port was found*/
    status = fbe_transport_get_status_code(packet);
    if((status != FBE_STATUS_OK) ||
    (sas_enclosure->ses_port_address == FBE_SAS_ADDRESS_INVALID))
    {
        retry_count = fbe_base_enclosure_increment_condition_retry_count((fbe_base_enclosure_t*)sas_enclosure);
        if ((retry_count%SAS_ENCLOSURE_LOG_SUPPRESS_COUNT) == 1)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_INFO,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "sas_encl_get_vport_addr_cond_compl Invalid packet stat:0x%X or no addr:0x%llX\n",
                                            status, (unsigned long long)sas_enclosure->ses_port_address);
        }
        
        /*
         * If we failed to get the virtual port address, check if we need to 
         * enter the reset ride through.
         */         
        fbe_sas_enclosure_check_reset_ride_through_needed((fbe_base_object_t*)sas_enclosure);
        
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /*
     * We have good status and got an address for our
     * virtual port.  It is safe to Clear the condition now. 
     */

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s GOOD status, setting SES port address: 0x%llX\n",
                                        __FUNCTION__, sas_enclosure->ses_port_address);

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "sas_encl_get_vport_addr_cond_compl can't clear current condition, status: 0x%X\n",
                                            status);

        
    }

    return FBE_STATUS_OK;

   

}/* end sas_enclosure_get_virtual_port_address_cond_completion */

/*!***************************************************************
 * @fn sas_enclosure_attach_smp_edge_cond_function(
 *                    fbe_base_object_t * p_object, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This condition function attaches the SMP edge.
 *
 * @param p_object - our object context
 * @param packet - request packet
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/08 - NC: created
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_enclosure_attach_smp_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = (fbe_sas_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry\n", __FUNCTION__);


    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_enclosure_common_cond_completion, sas_enclosure);

    /* Call the userper function to do actual job */
    fbe_sas_enclosure_attach_smp_edge(sas_enclosure, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

/*!***************************************************************
 * @fn sas_enclosure_open_smp_edge_cond_function(
 *                    fbe_base_object_t * p_object, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This condition function opens the smp edge.
 *
 * @param p_object - our object context
 * @param packet - request packet
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/08 - NC: created
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_enclosure_open_smp_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = (fbe_sas_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry \n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_enclosure_open_smp_edge_cond_completion, sas_enclosure);

    /* Call the userper function to do actual job */
    fbe_sas_enclosure_open_smp_edge(sas_enclosure, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}


/*!***************************************************************
 * @fn sas_enclosure_open_smp_edge_cond_completion(
 *                    fbe_packet_t * packet, 
 *                    fbe_packet_completion_context_t context)                  
 ****************************************************************
 * @brief
 *  This condition function handles the response from the usurper
 *  for the request to open a SMP edge.
 *
 * @param  packet - request packet
 * @param  context - what it says
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  7/28/08 - Created. bphilbin
 *
 ****************************************************************/
static fbe_status_t 
sas_enclosure_open_smp_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = NULL;
    fbe_path_attr_t path_attr;
    
    sas_enclosure = (fbe_sas_enclosure_t*)context;

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s Invalid packet status: 0x%X\n",
                                            __FUNCTION__, status);
       
        return FBE_STATUS_OK; /* this is completion OK */
    }

    status = fbe_sas_enclosure_get_smp_path_attributes(sas_enclosure, &path_attr);

    if((status == FBE_STATUS_OK) && (path_attr & FBE_SMP_PATH_ATTR_CLOSED))
    {
        /*
         * We failed to open the SMP edge for some reason.  We should
         * set the condition to refresh our virtual port address and check 
         * if we need to enter the reset ride through logic.
         */

        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_DEBUG_LOW,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s open SMP edge failed setting get SMP address condition\n", __FUNCTION__);

        status = fbe_lifecycle_set_cond(&fbe_sas_enclosure_lifecycle_const,
                                        (fbe_base_object_t*)sas_enclosure,
                                        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_SMP_ADDRESS);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                "%s, can't set GET SMP ADDRESS condition, status: 0x%x.\n",
                                __FUNCTION__, status);

            return FBE_STATUS_OK;
        }

        fbe_sas_enclosure_check_reset_ride_through_needed((fbe_base_object_t*)sas_enclosure);

        return FBE_STATUS_OK;
    }

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_DEBUG_LOW,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s open SMP edge succeeded\n", __FUNCTION__);

    /* We need to stop the glitch ride through by clearing the 
     * start time and expected period.
     */
    fbe_base_enclosure_set_time_ride_through_start((fbe_base_enclosure_t *)sas_enclosure, 0);
    fbe_base_enclosure_set_expected_ride_through_period((fbe_base_enclosure_t *)sas_enclosure, 0);

    /* I would assume that it is OK to clear CURRENT condition here.
        After all we are in condition completion context */

    /* open and get address conditions are paired up */
    fbe_base_enclosure_clear_condition_retry_count((fbe_base_enclosure_t*)sas_enclosure);

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s can't clear current condition, status: 0x%X\n",
                                            __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn sas_enclosure_attach_ssp_edge_cond_function(
 *                    fbe_base_object_t * p_object, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This condition function attaches the ssp (protocol) edge.
 *
 * @param p_object - our object context
 * @param packet - request packet
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/18/08 - NC: added header
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_enclosure_attach_ssp_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = (fbe_sas_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry\n", __FUNCTION__ );

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_enclosure_common_cond_completion, sas_enclosure);

    /* Call the userper function to do actual job */
    fbe_sas_enclosure_attach_ssp_edge(sas_enclosure, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

/*!***************************************************************
 * @fn sas_enclosure_open_ssp_edge_cond_function(
 *                    fbe_base_object_t * p_object, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This condition function opens the protocol (ssp) edge.
 *
 * @param p_object - our object context
 * @param packet - request packet
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/18/08 - NC: added header
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_enclosure_open_ssp_edge_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = (fbe_sas_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry \n", __FUNCTION__);


    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_enclosure_open_ssp_edge_cond_completion, sas_enclosure);

    /* Call the userper function to do actual job */
    fbe_sas_enclosure_open_ssp_edge(sas_enclosure, packet);

    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}


/*!***************************************************************
 * @fn sas_enclosure_open_ssp_edge_cond_completion(
 *                    fbe_packet_t * packet, 
 *                    fbe_packet_completion_context_t context)                  
 ****************************************************************
 * @brief
 *  This condition function handles the response from the usurper
 *  for the request to open a ssp (protocol) edge.
 *
 * @param  packet - request packet
 * @param  context - what it says
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  7/28/08 - Created. bphilbin
 *
 ****************************************************************/
static fbe_status_t 
sas_enclosure_open_ssp_edge_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = NULL;
    fbe_path_attr_t path_attr;
    
    sas_enclosure = (fbe_sas_enclosure_t*)context;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry \n", __FUNCTION__);

    /* Check the packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s Invalid packet status: 0x%X\n", 
                                            __FUNCTION__, status);

        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    status = fbe_sas_enclosure_get_ssp_path_attributes(sas_enclosure, &path_attr);

    if((status == FBE_STATUS_OK) && (path_attr & FBE_SSP_PATH_ATTR_CLOSED))
    {
        /*
         * We failed to open the ssp (protocol) edge for some reason.  We should
         * set the condition to refresh our virtual port address and check 
         * if we need to enter the reset ride through logic.
         */
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_DEBUG_LOW,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s failed to open ssp edge, setting GET_VIRTUAL_PORT_ADDR condition \n", __FUNCTION__);


        status = fbe_lifecycle_set_cond(&fbe_sas_enclosure_lifecycle_const,
                                        (fbe_base_object_t*)sas_enclosure,
                                        FBE_SAS_ENCLOSURE_LIFECYCLE_COND_GET_VIRTUAL_PORT_ADDRESS);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                "%s, can't set GET VIRTUAL PORT ADDRESS condition, status: 0x%x.\n",
                                __FUNCTION__, status);

            return FBE_STATUS_OK;
        }

        fbe_sas_enclosure_check_reset_ride_through_needed((fbe_base_object_t*)sas_enclosure);

        return FBE_STATUS_OK;
    }


    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_DEBUG_LOW,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s open SSP edge succeeded\n", __FUNCTION__);
    /* We need to stop the glitch ride through by clearing the 
     * start time and expected period.
     */
    fbe_base_enclosure_set_time_ride_through_start((fbe_base_enclosure_t *)sas_enclosure, 0);
    fbe_base_enclosure_set_expected_ride_through_period((fbe_base_enclosure_t *)sas_enclosure, 0);

    /* open and get address conditions are paired up */
    fbe_base_enclosure_clear_condition_retry_count((fbe_base_enclosure_t*)sas_enclosure);
    
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s can't clear current condition, status: 0x%X\n", 
                                            __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn sas_enclosure_inquiry_cond_function(
 *                    fbe_base_object_t * p_object, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This condition function generates inquiry through ssp (protocol) edge.
 *
 * @param p_object - our object context
 * @param packet - request packet
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/18/08 - NC: added header
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_enclosure_inquiry_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = (fbe_sas_enclosure_t*)p_object;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry\n", __FUNCTION__);

    /* Allocate a control operation. */
    payload = fbe_transport_get_payload_ex(packet);
    // The next operation is the control operation.
    control_operation = fbe_payload_ex_allocate_control_operation(payload);   

    if(control_operation == NULL)
    {    
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                        "%s allocate_control_operation failed.\n",
                        __FUNCTION__);

        // Set the packet status to FBE_STATUS_GENERIC_FAILURE.
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    
    }

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_enclosure_inquiry_cond_completion, sas_enclosure);

    /* Call the userper function to do actual job */
    status = fbe_sas_enclosure_send_inquiry(sas_enclosure, packet);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

/*!***************************************************************
 * @fn sas_enclosure_inquiry_cond_completion()
 *                    fbe_packet_t * packet, 
 *                    fbe_packet_completion_context_t context)                  
 ****************************************************************
 * @brief
 *  This condition function completes inquiry request.
 *
 * @param  packet - request packet
 * @param  context - what it says
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/18/08 - NC: added header
 *
 ****************************************************************/
static fbe_status_t 
sas_enclosure_inquiry_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_status_t ctrl_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    sas_enclosure = (fbe_sas_enclosure_t*)context;

    /* Check packet status */
    status = fbe_transport_get_status_code(packet);

    sas_enclosure = (fbe_sas_enclosure_t *)context;

    /* not to clear the condition if the packet failed, return from here instead. */
    if(status != FBE_STATUS_OK){
    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_ERROR,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s Invalid packet status: 0x%X\n", 
                                        __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    if(control_operation != NULL)
    { 
        // The control operation was allocated while sending down the command.
        fbe_payload_control_get_status(control_operation, &ctrl_status);

        if(ctrl_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
        {
            /* I would assume that it is OK to clear CURRENT condition here.
                After all we are in condition completion context */
            if(sas_enclosure->sasEnclosureProductType == FBE_SAS_ENCLOSURE_PRODUCT_TYPE_ESES)
            {
                fbe_base_object_change_class_id((fbe_base_object_t*)sas_enclosure, FBE_CLASS_ID_ESES_ENCLOSURE);
            }
            else
            {
                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                                    FBE_TRACE_LEVEL_ERROR,
                                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                                    "%s unsupported sas enclosure product id: 0x%X\n", 
                                                    __FUNCTION__, sas_enclosure->sasEnclosureProductType);

                /* update fault information and fail the enclosure */
                status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *) sas_enclosure, 
                                                0,
                                                0,
                                                FBE_ENCL_FLTSYMPT_UNSUPPORTED_ENCLOSURE,
                                                sas_enclosure->sasEnclosureProductType);

                // trace out the error if we can't handle the error
                if(!ENCL_STAT_OK(status))
                {
                    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                        FBE_TRACE_LEVEL_ERROR,
                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                        "%s, fbe_base_enclosure_handle_errors failed. status: 0x%x\n",
                        __FUNCTION__, status);
                }
            }

            /* Clear the current condition. */
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_enclosure);
            if (status != FBE_STATUS_OK) {
                fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                                    FBE_TRACE_LEVEL_ERROR,
                                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                                    "%s can't clear current condition, status: 0x%X\n", 
                                                    __FUNCTION__, status);
            }
        }
        //Release the control operation.
        fbe_payload_ex_release_control_operation(payload, control_operation);
    }
    else
    {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                            FBE_TRACE_LEVEL_ERROR,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                             "%s get_control_operation failed.\n",
                             __FUNCTION__);
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn sas_enclosure_get_element_list_cond_function(
 *                    fbe_base_object_t * p_object, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This condition function starts request to collect children list.
 *
 * @param p_object - our object context
 * @param packet - request packet
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/18/08 - NC: added header
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_enclosure_get_element_list_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = (fbe_sas_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_INFO,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_enclosure_get_element_list_cond_completion, sas_enclosure);

    /* Call the executer function to do actual job */
    status = fbe_sas_enclosure_send_get_element_list_command(sas_enclosure, packet);

    return FBE_LIFECYCLE_STATUS_PENDING;
}

/*!***************************************************************
 * @fn sas_enclosure_get_element_list_cond_completion()
 *                    fbe_packet_t * packet, 
 *                    fbe_packet_completion_context_t context)                  
 ****************************************************************
 * @brief
 *  This condition function completes the request to get children list
 *
 * @param  packet - request packet
 * @param  context - what it says
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/18/08 - NC: added header
 *
 ****************************************************************/
static fbe_status_t 
sas_enclosure_get_element_list_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = NULL;
    
    sas_enclosure = (fbe_sas_enclosure_t*)context;

    /* Check packet status */
    status = fbe_transport_get_status_code(packet);

    /* not to clear the condition if the packet failed, return from here instead. */
    if(status != FBE_STATUS_OK){
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s Invalid packet status: 0x%X\n", 
                                            __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s can't clear current condition, status: 0x%X\n", 
                                            __FUNCTION__, status);
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_sas_enclosure_check_reset_ride_through_needed(
 *                    fbe_base_object_t * p_object) 
 ****************************************************************
 * @brief
 *  This function checks the status of our edges to see if we
 *  should enter the reset ride through condition.
 *
 * @param p_object - our object context
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  8/6/08 - Created. bphilbin

 *
 ****************************************************************/
fbe_status_t
fbe_sas_enclosure_check_reset_ride_through_needed(fbe_base_object_t * p_object)
{
    fbe_status_t status;
    fbe_path_attr_t ssp_path_attr, disc_path_attr;
    fbe_path_state_t ssp_path_state, path_state;
    fbe_sas_enclosure_t * sas_enclosure = NULL;
    fbe_time_t time_ride_through_start;
    fbe_u32_t expected_ride_through_period = 0;
    fbe_lifecycle_state_t lifecycle_state;

    /********
     * BEGIN
     ********/
    sas_enclosure = (fbe_sas_enclosure_t*)p_object;

    /* Get all the states and attributes. If we see the path state as BROKEN then fail the enclosure
     * with FBE_ENCL_FLTSYMPT_UPSTREAM_ENCL_FAULTED.
     */
    status = fbe_base_discovered_get_path_state((fbe_base_discovered_t *) sas_enclosure, &path_state);
    if ( status != FBE_STATUS_OK || path_state == FBE_PATH_STATE_BROKEN )
    {
        /* update fault information and fail the enclosure */
        status = fbe_base_enclosure_handle_errors((fbe_base_enclosure_t *) sas_enclosure, 
                                        0,
                                        0,
                                        FBE_ENCL_FLTSYMPT_UPSTREAM_ENCL_FAULTED,
                                        sas_enclosure->sasEnclosureProductType);

        // trace out the error if we can't handle the error
        if(!ENCL_STAT_OK(status))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                "%s, Failed. status: 0x%x\n", __FUNCTION__, status);
        }

        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_enclosure);
        if (status != FBE_STATUS_OK) 
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                FBE_TRACE_LEVEL_ERROR,
                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                "%s can't clear current condition, status: 0x%X\n", 
                                __FUNCTION__, status);
        }

        return FBE_STATUS_OK;
    }

    /*
     * There's a timing window that discovery edge could be marked as not ENABLED
     * because the parent object goes to the activate state (for example, enclosure firmware activation).
     * We don't want to go to reset ride through for this object in this case.
     * we should check the status because the call below only returns OK status when the discovery edge is ENABLED.
     * This get us covered.
     */
    status = fbe_base_discovered_get_path_attributes((fbe_base_discovered_t*)sas_enclosure,
                                                    &disc_path_attr);

    /* We don't need to do anything if 
     * (1) The discovery edge is not enabled OR
     * (2) the discovery edge does not have NOT_PRESENT set OR
     * (3) the discovery edge has REMOVED set. In this case, both NOT_PRESENT and REMOVED are set.
     *     the object will be destroyed. The reset ride through is not needed).
     */
    if ((status != FBE_STATUS_OK) ||
        ((disc_path_attr & FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT)==0) ||
         (disc_path_attr & FBE_DISCOVERY_PATH_ATTR_REMOVED))
    {
        return FBE_STATUS_OK;
    }

    status = fbe_ssp_transport_get_path_state(&sas_enclosure->ssp_edge, &ssp_path_state);
    status = fbe_base_transport_get_path_attributes((fbe_base_edge_t *) (&sas_enclosure->ssp_edge), 
                                                    &ssp_path_attr);
    
    /* Get reset ride through related time values */
    fbe_base_enclosure_get_expected_ride_through_period((fbe_base_enclosure_t*)p_object, 
                                                        &expected_ride_through_period);
    fbe_base_enclosure_get_time_ride_through_start((fbe_base_enclosure_t*)p_object, 
                                                    &time_ride_through_start);
    /*
     * Check the status of our discovery and ssp (protocol) edges.  If they are both
     * enabled but not present then we kick off our reset ride through before 
     * destroying the enclosure object.
     *
     * We have already checked discovery edge attribute earlier.
     *
     * We only need to kick off the reset ride through if we haven't done so.
     *
     * Lost SMP edge will not trigger the ride through.
     */
    if ((status == FBE_STATUS_OK) &&
        (ssp_path_state == FBE_PATH_STATE_ENABLED) &&
        (ssp_path_attr & FBE_SSP_PATH_ATTR_CLOSED) &&
        (time_ride_through_start == 0))
    {

        status = fbe_lifecycle_get_state(&fbe_sas_enclosure_lifecycle_const,
                                    (fbe_base_object_t*)sas_enclosure,
                                    &lifecycle_state);

        if((status == FBE_STATUS_OK) && (lifecycle_state == FBE_LIFECYCLE_STATE_READY))
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                  FBE_TRACE_LEVEL_INFO,
                                  fbe_base_enclosure_get_logheader((fbe_base_enclosure_t *) sas_enclosure),
                                  "Changing state from READY to ACTIVATE. Reason:Expander is not available.\n");
        }
    
        // The enclosure will be in activate state after setting the following conditions.
        status = fbe_lifecycle_set_cond(&fbe_sas_enclosure_lifecycle_const,
                                        (fbe_base_object_t*)sas_enclosure,
                                        FBE_BASE_ENCLOSURE_LIFECYCLE_COND_INIT_RIDE_THROUGH);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                                "check_reset_ride_through_needed, set INIT_RIDE_THROUGH failed, status: 0x%x.\n", 
                                                 status);
            return status;
        }

        status = fbe_lifecycle_set_cond(&fbe_sas_enclosure_lifecycle_const,
                                        (fbe_base_object_t*)sas_enclosure,
                                        FBE_BASE_ENCLOSURE_LIFECYCLE_COND_RIDE_THROUGH_TICK);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                                FBE_TRACE_LEVEL_ERROR,
                                                fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                                "check_reset_ride_through_needed, set RIDE_THROUGH_TICK failed, status: 0x%x.\n", 
                                                status);
            return status;
        }
    }
    return FBE_STATUS_OK;
}// End of function fbe_sas_enclosure_check_reset_ride_through_needed

/*!***************************************************************
 * @fn sas_enclosure_detach_ssp_edge_cond_function(
 *                    fbe_base_object_t * p_object, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This condition function detach the ssp (protocol) edge.
 *
 * @param p_object - our object context
 * @param packet - request packet
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/18/08 - NC: added header
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_enclosure_detach_ssp_edge_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = (fbe_sas_enclosure_t *)base_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry\n", __FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_enclosure_common_cond_completion, sas_enclosure);

    fbe_sas_enclosure_detach_ssp_edge(sas_enclosure, packet);
    
    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}

/*!***************************************************************
 * @fn sas_enclosure_common_cond_completion()
 *                    fbe_packet_t * packet, 
 *                    fbe_packet_completion_context_t context)                  
 ****************************************************************
 * @brief
 *  This condition function completes the detach ssp (protocol) edge request
 *
 * @param  packet - request packet
 * @param  context - what it says
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/18/08 - NC: added header
 *
 ****************************************************************/
static fbe_status_t 
sas_enclosure_common_cond_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = NULL;
    
    sas_enclosure = (fbe_sas_enclosure_t *)context;


    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry\n", __FUNCTION__);
    /* Check packet status */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s Invalid packet status: 0x%X\n",
                                            __FUNCTION__, status);
        
        return FBE_STATUS_OK; /* this is completion OK */
    }

    /* I would assume that it is OK to clear CURRENT condition here.
        After all we are in condition completion context */
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_enclosure);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                            FBE_TRACE_LEVEL_ERROR,
                                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                            "%s can't clear current condition, status: 0x%X\n",
                                            __FUNCTION__, status);
        
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn sas_enclosure_detach_smp_edge_cond_function(
 *                    fbe_base_object_t * p_object, 
 *                    fbe_packet_t * packet)                  
 ****************************************************************
 * @brief
 *  This condition function detach the smp edge.
 *
 * @param p_object - our object context
 * @param packet - request packet
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - no error.
 *                 Other values imply there was an error.
 *
 * HISTORY:
 *  11/20/08 - NC: created
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
sas_enclosure_detach_smp_edge_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_enclosure_t * sas_enclosure = (fbe_sas_enclosure_t *)base_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_enclosure,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_enclosure),
                                        "%s entry\n",__FUNCTION__);

    /* We just push additional context on the monitor packet stack */
    status = fbe_transport_set_completion_function(packet, sas_enclosure_common_cond_completion, sas_enclosure);

    fbe_sas_enclosure_detach_smp_edge(sas_enclosure, packet);
    
    /* Return pending to the lifecycle */
    return FBE_LIFECYCLE_STATUS_PENDING;
}
