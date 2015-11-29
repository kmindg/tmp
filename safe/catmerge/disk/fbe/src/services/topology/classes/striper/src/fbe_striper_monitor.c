/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_striper_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the striper object lifecycle code.
 * 
 *  This includes the
 *  @ref fbe_striper_monitor_entry "striper monitor entry point", as well as all
 *  the lifecycle defines such as rotaries and conditions, along with the actual condition
 *  functions.
 * 
 * @ingroup striper_class_files
 * 
 * @version
 *   07/21/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_striper.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_config_private.h"
#include "fbe_striper_private.h"


/*!***************************************************************
 * fbe_striper_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the striper's monitor.
 *
 * @param object_handle - This is the object handle, or in our case the pdo.
 * @param packet_p - The packet arriving from the monitor scheduler.
 *
 * @return fbe_status_t - Status of monitor operation.             
 *
 * @author
 *  5/20/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t 
fbe_striper_monitor_entry(fbe_object_handle_t object_handle, 
                              fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_striper_t * striper_p = NULL;

    striper_p = (fbe_striper_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)striper_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_base_config_monitor_crank_object(&fbe_striper_lifecycle_const, 
                                        (fbe_base_object_t*)striper_p, packet_p);
    return status;
}
/******************************************
 * end fbe_striper_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_striper_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the striper.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the striper's constant
 *                        lifecycle data.
 *
 * @author
 *  07/21/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_striper_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(striper));
}
/******************************************
 * end fbe_striper_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t striper_downstream_health_not_optimal_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_lifecycle_status_t striper_downstream_health_disabled_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);
static fbe_lifecycle_status_t striper_downstream_health_broken_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(striper);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(striper);

/*  striper_lifecycle_callbacks
 *  This variable has all the callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(striper) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        striper,
        FBE_LIFECYCLE_NULL_FUNC,          /* online function */
        FBE_LIFECYCLE_NULL_FUNC)   /* pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/

static fbe_lifecycle_const_cond_t striper_downstream_health_broken_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN,
        striper_downstream_health_broken_cond_function)
};

static fbe_lifecycle_const_cond_t striper_downstream_health_disabled_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED,
        striper_downstream_health_disabled_cond_function)
};

/* We override this, but for now the implementation is the same as health disabled.
 */
static fbe_lifecycle_const_cond_t striper_downstream_health_not_optimal_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_NOT_OPTIMAL,
        striper_downstream_health_not_optimal_cond_function)
};


/*--- constant base condition entries --------------------------------------------------*/


/* striper_lifecycle_base_cond_array
 * This is our static list of base conditions.
 */
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(striper)[] CSX_MAYBE_UNUSED = {
    NULL /* empty for now. */
};

/* striper_lifecycle_base_conditions 
 *  This is the list of base default base conditions for the logical
 *  drive.
 */
//FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(striper);

/*! @todo For now there are no base conditions.  Uncomment the above if we add them.
 */
FBE_LIFECYCLE_DEF_CONST_NULL_BASE_CONDITIONS(striper);

/*--- constant rotaries ----------------------------------------------------------------*/
static fbe_lifecycle_const_rotary_cond_t striper_specialize_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(striper_downstream_health_not_optimal_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    /* Base conditions */
};

static fbe_lifecycle_const_rotary_cond_t striper_activate_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(striper_downstream_health_disabled_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
        
    /* Base conditions */
};

static fbe_lifecycle_const_rotary_cond_t striper_fail_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(striper_downstream_health_broken_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /* Base conditions */
};

static fbe_lifecycle_const_rotary_cond_t striper_destroy_rotary[] = {
    /* Derived conditions */
    NULL /* empty for now. */
    /* Base conditions */
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(striper)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, striper_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, striper_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_FAIL, striper_fail_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(striper);


/*--- global striper lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(striper) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        striper,
        FBE_CLASS_ID_STRIPER,                   /* This class */
        FBE_LIFECYCLE_CONST_DATA(raid_group))  /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/

/*--- Condition Functions --------------------------------------------------------------*/


/*!**************************************************************
 * striper_downstream_health_broken_cond_function()
 ****************************************************************
 * @brief
 *  This is the derived condition function for striper object 
 *  where it will clear this condition when it finds health with
 *  downstream object is either ENABLE or DEGRADED.
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is be based on parent
 *                                  function call.
 * call.
 *
 * @author
 *  8/18/2009 - Created. Dhaval Patel.
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
striper_downstream_health_broken_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_striper_t * striper_p = (fbe_striper_t *)object_p;
    fbe_status_t status;
    fbe_base_config_downstream_health_state_t downstream_health_state;

    fbe_base_object_trace((fbe_base_object_t*)striper_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Look for all the edges with downstream objects.
       */
    downstream_health_state = fbe_striper_verify_downstream_health(striper_p);

    switch (downstream_health_state)
    {
        case FBE_DOWNSTREAM_HEALTH_OPTIMAL:
        case FBE_DOWNSTREAM_HEALTH_DISABLED:
            /* Currently clear the condition in both the cases, later logic
             * will be changed to integrate different use cases.
             */
            status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)striper_p);
            if (status != FBE_STATUS_OK) 
            {
                fbe_base_object_trace((fbe_base_object_t*)striper_p, 
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s can't clear current condition, status: 0x%X\n",
                                        __FUNCTION__, status);
            }
            /* Jump to Activate state */            
            fbe_lifecycle_set_cond(&fbe_striper_lifecycle_const, (fbe_base_object_t *)striper_p, FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);

            break;
        case FBE_DOWNSTREAM_HEALTH_BROKEN:
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t*)striper_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Invalid downstream health state: 0x%x!!\n", 
                                  __FUNCTION__,
                                  downstream_health_state);
            break;
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}
/*************************************************
 * end striper_downstream_health_broken_cond_function()
 *************************************************/

static fbe_lifecycle_status_t 
striper_downstream_health_disabled_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_striper_t * striper_p = (fbe_striper_t*)object_p;
    fbe_base_config_downstream_health_state_t downstream_health_state;

    fbe_base_object_trace((fbe_base_object_t*)striper_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    downstream_health_state = fbe_striper_verify_downstream_health(striper_p);
    switch(downstream_health_state)
    {
        case FBE_DOWNSTREAM_HEALTH_BROKEN:
        {
            /* set downstream health broken condition. */
            fbe_lifecycle_set_cond(&fbe_striper_lifecycle_const,
                                   (fbe_base_object_t *)striper_p,
                                   FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);        
            break;
        }
        case FBE_DOWNSTREAM_HEALTH_OPTIMAL :
        {
            fbe_lifecycle_clear_current_cond((fbe_base_object_t*)striper_p); 
            break;   
        }
        default:
            break;
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;
}


static fbe_lifecycle_status_t 
striper_downstream_health_not_optimal_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_striper_t * striper_p = (fbe_striper_t*)object_p;
    fbe_base_config_downstream_health_state_t downstream_health_state;
	fbe_lifecycle_status_t lifecycle_status;

    fbe_base_object_trace((fbe_base_object_t*)striper_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

	lifecycle_status = fbe_raid_group_monitor_push_keys_if_needed(object_p, packet_p);
	if(lifecycle_status != FBE_LIFECYCLE_STATUS_CONTINUE){
		return lifecycle_status;
	}

    downstream_health_state = fbe_striper_verify_downstream_health(striper_p);
    switch(downstream_health_state)
    {
        case FBE_DOWNSTREAM_HEALTH_OPTIMAL :
        {
            fbe_lifecycle_clear_current_cond((fbe_base_object_t*)striper_p); 
            break;   
        }
        default:
            break;
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;
}

/*******************************
 * end fbe_striper_monitor.c
 *******************************/


