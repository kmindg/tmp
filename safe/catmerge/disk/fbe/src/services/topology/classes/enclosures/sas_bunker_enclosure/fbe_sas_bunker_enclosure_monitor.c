/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_sas_bunker_enclosure_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains monitor functions for sas bunker enclosure.
 *
 * @ingroup fbe_enclosure_class
 *
 * HISTORY
 *    08-Jun-2009 : Dipak Patel - Added header.
 *    08-Jun-2009  : Dipak Patel - Created from fbe_sas_viper_enclosure_monitor.c
 *
 ***************************************************************************/

#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe_transport_memory.h"
#include "sas_bunker_enclosure_private.h"

/*!*************************************************************************
* @fn fbe_sas_bunker_enclosure_monitor_entry(                  
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
*    08-Jun-2009 : Dipak Patel - Added header.
*    08-Jun-2009 : Dipak Patel - Created from fbe_sas_magnum_enclosure_monitor_entry.
*
***************************************************************************/
fbe_status_t 
fbe_sas_bunker_enclosure_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_sas_bunker_enclosure_t * sas_bunker_enclosure = NULL;

    sas_bunker_enclosure = (fbe_sas_bunker_enclosure_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bunker_enclosure,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bunker_enclosure),
                            "%s entry .\n", __FUNCTION__);

    status = fbe_lifecycle_crank_object(&fbe_sas_bunker_enclosure_lifecycle_const, (fbe_base_object_t*)sas_bunker_enclosure, packet);

    return status;
}

fbe_status_t fbe_sas_bunker_enclosure_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(sas_bunker_enclosure));
}

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t 
sas_bunker_enclosure_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

static fbe_lifecycle_status_t 
sas_bunker_enclosure_foo_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

/*--- local function prototypes --------------------------------------------------------*/

/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(sas_bunker_enclosure);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(sas_bunker_enclosure);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(sas_bunker_enclosure) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        sas_bunker_enclosure,
        FBE_LIFECYCLE_NULL_FUNC,       /* online function */
        FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/
static fbe_lifecycle_const_cond_t sas_bunker_enclosure_self_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_OBJECT_LIFECYCLE_COND_SELF_INIT,
        sas_bunker_enclosure_self_init_cond_function)
};
/*--- constant base condition entries --------------------------------------------------*/
/* FBE_SAS_MAGNUM_LIFECYCLE_COND_FOO condition */
static fbe_lifecycle_const_base_cond_t sas_bunker_enclosure_foo_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "sas_bunker_enclosure_foo_cond",
        FBE_SAS_BUNKER_LIFECYCLE_COND_FOO,
        sas_bunker_enclosure_foo_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
        FBE_LIFECYCLE_STATE_READY,         /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)             /* DESTROY */
};

/* 
 * The conditions here in the BASE_COND_ARRY must match the condition order 
 * in the fbe_sas_bunker_enclosure_lifecycle_cond_id_e enum.
 */
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(sas_bunker_enclosure)[] = {
    &sas_bunker_enclosure_foo_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(sas_bunker_enclosure);

/*--- constant rotaries ----------------------------------------------------------------*/
static fbe_lifecycle_const_rotary_cond_t sas_bunker_enclosure_specialize_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_bunker_enclosure_self_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /* Base conditions */
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(sas_bunker_enclosure)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, sas_bunker_enclosure_specialize_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(sas_bunker_enclosure);

/*--- global sas bunker enclosure lifecycle constant data ----------------------------------------*/
fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_bunker_enclosure) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        sas_bunker_enclosure,
        FBE_CLASS_ID_SAS_BUNKER_ENCLOSURE,          /* This class */
        FBE_LIFECYCLE_CONST_DATA(eses_enclosure))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/


/*--- Condition Functions --------------------------------------------------------------*/


/*!*************************************************************************
* @fn sas_bunker_enclosure_self_init_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition completes the initialization of bunker enclosure object.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*    08-Jun-2009 : Dipak Patel - Added header.
*    08-Jun-2009 : Dipak Patel - Created from fbe_sas_magnum_enclosure_self_init_cond_function.
*
***************************************************************************/
static fbe_lifecycle_status_t 
sas_bunker_enclosure_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_sas_bunker_enclosure_t * sas_bunker_enclosure = (fbe_sas_bunker_enclosure_t*)p_object;

    fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bunker_enclosure,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bunker_enclosure),
                          "%s entry\n", __FUNCTION__);

     
    status = fbe_sas_bunker_enclosure_init(sas_bunker_enclosure);

    /* If we don't check here, we could cause panic */
    if (status == FBE_STATUS_OK) {
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_bunker_enclosure);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_customizable_trace((fbe_base_object_t*)sas_bunker_enclosure, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    fbe_base_enclosure_get_logheader((fbe_base_enclosure_t*)sas_bunker_enclosure),
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    return FBE_LIFECYCLE_STATUS_DONE;
}


/*!*************************************************************************
* @fn sas_bunker_enclosure_foo_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition does absolutely nothing.  It exists so that this
*       enclosure class has a condition.  If any other condition is added
*       in the future, this condition should be removed.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*    08-Jun-2009 : Dipak Patel - Added header.
*    08-Jun-2009 : Dipak Patel - Created from sas_viper_enclosure_foo_cond_function.
*
***************************************************************************/
static fbe_lifecycle_status_t 
sas_bunker_enclosure_foo_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    return FBE_LIFECYCLE_STATUS_DONE;
}
// End of file

