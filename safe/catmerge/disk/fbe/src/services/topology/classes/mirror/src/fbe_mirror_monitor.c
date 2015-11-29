/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_mirror_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the mirror object lifecycle code.
 * 
 *  This includes the
 *  @ref fbe_mirror_monitor_entry "mirror monitor entry point", as well as all
 *  the lifecycle defines such as rotaries and conditions, along with the actual condition
 *  functions.
 * 
 * @ingroup mirror_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_mirror.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_config_private.h"
#include "fbe_mirror_private.h"
#include "fbe_raid_group_needs_rebuild.h"               //  for fbe_raid_group_needs_rebuild_process_nr_on_demand()


/*!***************************************************************
 * fbe_mirror_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the mirror's monitor.
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
fbe_mirror_monitor_entry(fbe_object_handle_t object_handle, 
                              fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_mirror_t * mirror_p = NULL;

    mirror_p = (fbe_mirror_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)mirror_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_base_config_monitor_crank_object(&fbe_mirror_lifecycle_const, 
                                        (fbe_base_object_t*)mirror_p, packet_p);
    return status;
}
/******************************************
 * end fbe_mirror_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_mirror_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the mirror.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the mirror's constant
 *                        lifecycle data.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_mirror_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(mirror));
}
/******************************************
 * end fbe_mirror_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t mirror_downstream_health_not_optimal_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(mirror);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(mirror);

/*  mirror_lifecycle_callbacks
 *  This variable has all the callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(mirror) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
        mirror,
        FBE_LIFECYCLE_NULL_FUNC,          /* online function */
        FBE_LIFECYCLE_NULL_FUNC)          /* pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/

/* We override this condition and for now its implementation is the same 
 * as downstream health disabled. 
 */
static fbe_lifecycle_const_cond_t mirror_downstream_health_not_optimal_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_NOT_OPTIMAL,
        mirror_downstream_health_not_optimal_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/


/* mirror_lifecycle_base_cond_array
 * This is our static list of base conditions.
 */
static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(mirror)[] CSX_MAYBE_UNUSED = {
    NULL /* empty for now. */
};

/* mirror_lifecycle_base_conditions 
 *  This is the list of base default base conditions for the logical
 *  drive.
 */
//FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(mirror);

/*! @todo For now there are no base conditions.  Uncomment the above if we add them.
 */
FBE_LIFECYCLE_DEF_CONST_NULL_BASE_CONDITIONS(mirror);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t mirror_specialize_rotary[] = {
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(mirror_downstream_health_not_optimal_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(mirror)[] = {    
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, mirror_specialize_rotary),   
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(mirror);


/*--- global mirror lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(mirror) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
        mirror,
        FBE_CLASS_ID_MIRROR,                   /* This class */
        FBE_LIFECYCLE_CONST_DATA(raid_group))  /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/

/*--- Condition Functions --------------------------------------------------------------*/

static fbe_lifecycle_status_t 
mirror_downstream_health_not_optimal_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_mirror_t *                              mirror_p = (fbe_mirror_t*)object_p;
    fbe_base_config_downstream_health_state_t   downstream_health_state;
	fbe_lifecycle_status_t lifecycle_status;
    

    fbe_base_object_trace((fbe_base_object_t*)mirror_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

	lifecycle_status = fbe_raid_group_monitor_push_keys_if_needed(object_p, packet_p);
	if(lifecycle_status != FBE_LIFECYCLE_STATUS_CONTINUE){
		return lifecycle_status;
	}


    downstream_health_state = fbe_raid_group_verify_downstream_health((fbe_raid_group_t*)mirror_p);
    if((downstream_health_state == FBE_DOWNSTREAM_HEALTH_OPTIMAL) ||
        (downstream_health_state == FBE_DOWNSTREAM_HEALTH_DEGRADED)){
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)mirror_p);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}



/*******************************
 * end fbe_mirror_monitor.c
 *******************************/
