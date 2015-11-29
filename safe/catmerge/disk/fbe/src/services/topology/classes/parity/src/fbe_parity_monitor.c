/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity_monitor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the parity object lifecycle code.
 * 
 *  This includes the
 *  @ref fbe_parity_monitor_entry "parity monitor entry point", as well as all
 *  the lifecycle defines such as rotaries and conditions, along with the actual condition
 *  functions.
 * 
 * @ingroup parity_class_files
 * 
 * @version
 *   07/21/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_parity.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_config_private.h"
#include "fbe_parity_private.h"
#include "fbe_raid_group_needs_rebuild.h"               //  for fbe_raid_group_needs_rebuild_process_nr_on_demand()


/*!***************************************************************
 * fbe_parity_monitor_entry()
 ****************************************************************
 * @brief
 *  Entry routine for the parity's monitor.
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
fbe_parity_monitor_entry(fbe_object_handle_t object_handle, 
                         fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_parity_t * parity_p = NULL;

    parity_p = (fbe_parity_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)parity_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_base_config_monitor_crank_object(&fbe_parity_lifecycle_const, 
                                        (fbe_base_object_t*)parity_p, packet_p);
    return status;
}
/******************************************
 * end fbe_parity_monitor_entry()
 ******************************************/

/*!**************************************************************
 * fbe_parity_monitor_load_verify()
 ****************************************************************
 * @brief
 *  This function simply validates the constant lifecycle data
 *  that is associated with the parity.
 *
 * @param  - None.               
 *
 * @return fbe_status_t - The status of the call to
 *                        fbe_lifecycle_class_const_verify()
 *                        for the parity's constant
 *                        lifecycle data.
 *
 * @author
 *  07/21/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_parity_monitor_load_verify(void)
{
    return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(parity));
}
/******************************************
 * end fbe_parity_monitor_load_verify()
 ******************************************/

/*--- local function prototypes --------------------------------------------------------*/

static fbe_lifecycle_status_t fbe_parity_monitor_eval_journal_flush_cond_function(fbe_base_object_t* in_object_p,
                                                                                  fbe_packet_t*      in_packet_p);

static fbe_lifecycle_status_t 
parity_downstream_health_not_optimal_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

static fbe_lifecycle_status_t
fbe_parity_init_journal_write_log_info_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(parity);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(parity);

/*  parity_lifecycle_callbacks
 *  This variable has all the callbacks the lifecycle needs.
 */
static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(parity) = {
    FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
                                     parity,
                                     FBE_LIFECYCLE_NULL_FUNC,    /* online function */
                                     FBE_LIFECYCLE_NULL_FUNC)    /* pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/

static fbe_lifecycle_const_cond_t parity_eval_flush_journal_cond =
{
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_JOURNAL_FLUSH,
                                         fbe_parity_monitor_eval_journal_flush_cond_function)
};

static fbe_lifecycle_const_cond_t parity_downstream_health_not_optimal_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_NOT_OPTIMAL,
                                         parity_downstream_health_not_optimal_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/

/*! @todo For now there are no base conditions.  Uncomment the above if we add them.
 */
FBE_LIFECYCLE_DEF_CONST_NULL_BASE_CONDITIONS(parity);

/*--- constant rotaries ----------------------------------------------------------------*/

/*  Specialize state rotary */
static fbe_lifecycle_const_rotary_cond_t parity_specialize_rotary[] = 
{
    /* Derived conditions */
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(parity_downstream_health_not_optimal_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),  
    /* Base conditions */
};

/*  Activate state rotary */
static fbe_lifecycle_const_rotary_cond_t parity_activate_rotary[] = 
{
    /* Derived conditions */    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(parity_eval_flush_journal_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),

    /* Base conditions */
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(parity)[] = 
{
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, parity_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, parity_activate_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(parity);


/*--- global parity lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(parity) = {
    FBE_LIFECYCLE_DEF_CONST_DATA(
                                parity,
                                FBE_CLASS_ID_PARITY,    /* This class */
                                FBE_LIFECYCLE_CONST_DATA(raid_group))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/

/*--- Condition Functions --------------------------------------------------------------*/

/*!**************************************************************
 * fbe_parity_monitor_eval_journal_flush_cond_function()
 ****************************************************************
 * @brief
 *  This derived condition function is used to detect if a journal
 *  flush needs to start on this SP after an unexpected restart
 *  when writes were oustanding to a degraded array.  In order to
 *  guarantee consistency and correctness the journaled writes
 *  must be commited to the drives before any data is reconstructed
 *  for the affected rows or a rebuild starts.
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is be based on parent
 *                                  function call.
 *
 * @author
 *  1/04/2010 - Daniel Cummins
 *
 ****************************************************************/
static fbe_lifecycle_status_t
fbe_parity_monitor_eval_journal_flush_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_bool_t         is_active_b;
    fbe_bool_t         is_flush_req_b;
    fbe_parity_t * parity_p = (fbe_parity_t *)object_p;
    fbe_raid_group_t * raid_group_p = (fbe_raid_group_t *)object_p;

    /* Set metadata_element attribute to not release data stripe locks on peer_contact_lost 
     * (dualsp write_logging support) 
     */
    fbe_metadata_element_hold_peer_data_stripe_locks_on_contact_loss(&parity_p->raid_group.base_config.metadata_element);

	/* Check if Flush is required */
	is_flush_req_b = fbe_raid_group_is_write_log_flush_required(raid_group_p); 

    fbe_raid_group_monitor_check_active_state(raid_group_p, &is_active_b);

    /* this condition is only valid if it was degraded, and this is the active SP */
    if ((is_flush_req_b == FBE_TRUE)
        && (is_active_b == FBE_TRUE))
    {
        /* Set all write_log slots as valid as this SP will do the Flushes (see function description) */
        fbe_parity_write_log_set_all_slots_valid(parity_p->write_log_info_p, FBE_TRUE);
        
        /* set the do_journal_flush condition in the raid group object */
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, 
                               object_p,
                               FBE_RAID_GROUP_LIFECYCLE_COND_JOURNAL_FLUSH);

        fbe_base_object_trace(object_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_activate: JOURNAL FLUSH condition set\n");
    }
    else
    {
        /* Set all write_log slots as free. Active SP would flush all slots before object goes to Ready */
        fbe_parity_write_log_set_all_slots_free(parity_p->write_log_info_p, FBE_TRUE);

        fbe_base_object_trace(object_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_activate: JOURNAL FLUSH condition skipped. is_flush_required %d is_active %d \n", is_flush_req_b, is_active_b);
    }

    /* Sucessfully cleared the condition, we are done. 
     */
    fbe_lifecycle_clear_current_cond(object_p);

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!**************************************************************
 * parity_downstream_health_not_optimal_cond_function()
 ****************************************************************
 * @brief
 *
 * @param object_p - The ptr to the base object of the base config.
 * @param packet_p - The packet sent to us from the scheduler.
 *
 * @return fbe_lifecycle_status_t - Return is be based on parent
 *                                  function call.
 *
 * @author
 *  04/12/2012 - Created
 *
 ****************************************************************/
static fbe_lifecycle_status_t 
parity_downstream_health_not_optimal_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p)
{
    fbe_parity_t *                              parity_p = (fbe_parity_t*)object_p;
    fbe_base_config_downstream_health_state_t   downstream_health_state;
	fbe_lifecycle_status_t lifecycle_status;

    fbe_base_object_trace((fbe_base_object_t*)parity_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);



	lifecycle_status = fbe_raid_group_monitor_push_keys_if_needed(object_p, packet_p);
	if(lifecycle_status != FBE_LIFECYCLE_STATUS_CONTINUE){ 
		return lifecycle_status;
	}

    downstream_health_state = fbe_raid_group_verify_downstream_health((fbe_raid_group_t*)parity_p);
    if((downstream_health_state == FBE_DOWNSTREAM_HEALTH_OPTIMAL) ||
        (downstream_health_state == FBE_DOWNSTREAM_HEALTH_DEGRADED)){
        fbe_lifecycle_clear_current_cond((fbe_base_object_t*)parity_p);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*******************************
 * end fbe_parity_monitor.c
 *******************************/
