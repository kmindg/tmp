/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_sep_hooks.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions for setting, checking and removing debug hooks
 *  when using the the Storage Extent Package (SEP).
 * 
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "sep_tests.h"
#include "sep_utils.h"
#include "sep_hook.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_cmi_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_job_service_interface.h"


/*****************************************
 * DEFINITIONS
 *****************************************/

/*!***************************************************************************
 *          fbe_test_add_debug_hook_active()
 *****************************************************************************
 *  
 *  @brief  Add the debug hook for the object specified on the active (for the
 *          object SP). 
 *
 * @param   object_id - object id to wait for
 * @param   state - the state in the monitor 
 * @param   substate - the substate in the monitor 
 * @param   val1 - first 64-bit value ot check
 * @param   val2 - second 64-bit value ot check 
 * @param   check_type  - check type (state, etc)
 * @param   action - log, pause etc
 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  12/04/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_add_debug_hook_active(fbe_object_id_t object_id, 
                                            fbe_u32_t state, 
                                            fbe_u32_t substate,
                                            fbe_u64_t val1,
                                            fbe_u64_t val2,
                                            fbe_u32_t check_type,
                                            fbe_u32_t action)

{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_metadata_info_t                     metadata_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the active SP for this object.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_base_config_metadata_get_info(object_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the metadata element state for this SP.
     */
    if (metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE)
    {
        active_sp = this_sp;
    }
    else
    {
        /* Else the active is the `other' SP.  Validate that the other SP
         * is alive.
         */
        active_sp = other_sp;
        status = fbe_api_cmi_service_get_info(&cmi_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (cmi_info.peer_alive == FBE_FALSE)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s for obj: 0x%x active SP: %d not alive", 
                       __FUNCTION__, object_id, active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    }

    /* Add the debug hook on the active.
     */
    if (active_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    status = fbe_api_scheduler_add_debug_hook(object_id,
                                              state,
                                              substate,
                                              val1,
                                              val2,
                                              check_type,
                                              action);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we changed SP change it back.
     */
    if (active_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Log a message.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "add hook active: %d obj: 0x%x state: %d substate: %d", 
               active_sp, object_id, state, substate);

    return status;
}
/**************************************
 * end fbe_test_add_debug_hook_active()
 **************************************/

/*!***************************************************************************
 *          fbe_test_add_debug_hook_passive()
 *****************************************************************************
 *  
 *  @brief  Add the debug hook for the object specified on the passive (for the
 *          object SP). 
 *
 * @param   object_id - object id to wait for
 * @param   state - the state in the monitor 
 * @param   substate - the substate in the monitor
 * @param   val1 - first 64-bit value ot check
 * @param   val2 - second 64-bit value ot check  
 * @param   check_type  - check type (state, etc)
 * @param   action - log, pause etc
 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  12/04/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_add_debug_hook_passive(fbe_object_id_t object_id, 
                                            fbe_u32_t state, 
                                            fbe_u32_t substate,
                                            fbe_u64_t val1,
                                            fbe_u64_t val2,
                                            fbe_u32_t check_type,
                                            fbe_u32_t action)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_metadata_info_t                     metadata_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   passive_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the active SP for this object.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_base_config_metadata_get_info(object_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the metadata element state for this SP.
     */
    if (metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE)
    {
        passive_sp = this_sp;
    }
    else
    {
        /* Else the passive is the `other' SP.  Validate that the other SP
         * is alive.
         */
        passive_sp = other_sp;
        status = fbe_api_cmi_service_get_info(&cmi_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (cmi_info.peer_alive == FBE_FALSE)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s for obj: 0x%x passive SP: %d not alive", 
                       __FUNCTION__, object_id, passive_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    }

    /* Add the debug hook on the passive.
     */
    if (passive_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(passive_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    status = fbe_api_scheduler_add_debug_hook(object_id,
                                              state,
                                              substate,
                                              val1,
                                              val2,
                                              check_type,
                                              action);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we changed SP change it back.
     */
    if (passive_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Log a message.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "add hook passive: %d obj: 0x%x state: %d substate: %d", 
               passive_sp, object_id, state, substate);

    return status;
}
/***************************************
 * end fbe_test_add_debug_hook_passive()
 ***************************************/
static fbe_u32_t fbe_test_timeout_ms = FBE_TEST_HOOK_WAIT_MSEC;
fbe_u32_t fbe_test_get_timeout_ms(void)
{
    return fbe_test_timeout_ms;
}
void fbe_test_set_timeout_ms(fbe_u32_t time_ms)
{
    fbe_test_timeout_ms = time_ms;
}
/*!**************************************************************
* fbe_test_wait_for_debug_hook()
****************************************************************
* @brief
*  Wait for the hook to be hit.
*
* @param object_id - object id to wait for
*        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
* @return fbe_status_t - status 
 *
* @author
*  3/29/2012 - Created. Rob Foley
*
****************************************************************/
fbe_status_t fbe_test_wait_for_debug_hook(fbe_object_id_t object_id, 
                                        fbe_u32_t state, 
                                        fbe_u32_t substate,
                                        fbe_u32_t check_type,
                                        fbe_u32_t action,
                                        fbe_u64_t val1,
                                        fbe_u64_t val2) 
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = fbe_test_get_timeout_ms();
    
    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.object_id = object_id;
    hook.check_type = check_type;
    hook.action = action;
    hook.val1 = val1;
    hook.val2 = val2;
    hook.counter = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the hook. objid: 0x%x state: %d substate: %d.", 
               object_id, state, substate);

    while (current_time < timeout_ms){

        status = fbe_api_scheduler_get_debug_hook(&hook);

        if (hook.counter != 0) {
            mut_printf(MUT_LOG_TEST_STATUS, "We have hit the debug hook objid: 0x%x state: %d substate: %d.", 
                       object_id, state, substate);
            return status;
        }
        current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object 0x%x did not hit state %d substate %d in %d ms!\n", 
                  __FUNCTION__, object_id, state, substate, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
* end fbe_test_wait_for_debug_hook()
******************************************/

/*!***************************************************************************
 *          fbe_test_wait_for_debug_hook_active()
 *****************************************************************************
 *  
 *  @brief  Wait for the debug hook for the object specified on the active 
 *          (for the object SP). 
 *
 * @param   object_id - object id to wait for
 * @param   state - the state in the monitor 
 * @param   substate - the substate in the monitor  
 * @param   check_type  - check type (state, etc)
 * @param   action - log, pause etc
 * @param   val1 - first 64-bit value ot check
 * @param   val2 - second 64-bit value ot check
 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  12/05/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_wait_for_debug_hook_active(fbe_object_id_t object_id, 
                                                 fbe_u32_t state, 
                                                 fbe_u32_t substate,
                                                 fbe_u32_t check_type,
                                                 fbe_u32_t action,
                                                 fbe_u64_t val1,
                                                 fbe_u64_t val2) 
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_metadata_info_t                     metadata_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the active SP for this object.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_base_config_metadata_get_info(object_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the metadata element state for this SP.
     */
    if (metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE)
    {
        active_sp = this_sp;
    }
    else
    {
        /* Else the active is the `other' SP.  Validate that the other SP
         * is alive.
         */
        active_sp = other_sp;
        status = fbe_api_cmi_service_get_info(&cmi_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (cmi_info.peer_alive == FBE_FALSE)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s for obj: 0x%x active SP: %d not alive", 
                       __FUNCTION__, object_id, active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    }

    /* Add the debug hook on the active.
     */
    if (active_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Wait until we hit the hook.
     */
    status = fbe_test_wait_for_debug_hook(object_id, 
                                          state,
                                          substate,
                                          check_type, 
                                          action, 
                                          val1,
                                          val2);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we changed SP change it back.
     */
    if (active_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return status;
}
/******************************************
* end fbe_test_wait_for_debug_hook_active()
******************************************/

/*!***************************************************************************
 *          fbe_test_wait_for_debug_hook_passive()
 *****************************************************************************
 *  
 *  @brief  Wait for the debug hook for the object specified on the passive 
 *          (for the object SP). 
 *
 * @param   object_id - object id to wait for
 * @param   state - the state in the monitor 
 * @param   substate - the substate in the monitor  
 * @param   check_type  - check type (state, etc)
 * @param   action - log, pause etc
 * @param   val1 - first 64-bit value ot check
 * @param   val2 - second 64-bit value ot check
 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  12/05/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_wait_for_debug_hook_passive(fbe_object_id_t object_id, 
                                                  fbe_u32_t state, 
                                                  fbe_u32_t substate,
                                                  fbe_u32_t check_type,
                                                  fbe_u32_t action,
                                                  fbe_u64_t val1,
                                                  fbe_u64_t val2) 
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_metadata_info_t                     metadata_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   passive_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the active SP for this object.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_base_config_metadata_get_info(object_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the metadata element state for this SP.
     */
    if (metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE)
    {
        passive_sp = this_sp;
    }
    else
    {
        /* Else the passive is the `other' SP.  Validate that the other SP
         * is alive.
         */
        passive_sp = other_sp;
        status = fbe_api_cmi_service_get_info(&cmi_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (cmi_info.peer_alive == FBE_FALSE)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s for obj: 0x%x passive SP: %d not alive", 
                       __FUNCTION__, object_id, passive_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    }

    /* Add the debug hook on the passive.
     */
    if (passive_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(passive_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Wait until we hit the hook.
     */
    status = fbe_test_wait_for_debug_hook(object_id, 
                                          state,
                                          substate,
                                          check_type, 
                                          action, 
                                          val1,
                                          val2);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we changed SP change it back.
     */
    if (passive_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return status;
}
/*******************************************
* end fbe_test_wait_for_debug_hook_passive()
*******************************************/

/*!**************************************************************
 * fbe_test_get_debug_hook()
 ****************************************************************
 * @brief
 *  Get the hook information.
 *
 * @param rg_object_id - the rg object id to wait for
 *                       The rest of the parameters are the
 *                       hook parameters we are searching for.
 *        state - 
 *        substate - 
 *        check_type - 
 *        action - 
 *        val1
 *        val2
 *        hook_p - Output information on this hook.
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  3/29/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_test_get_debug_hook(fbe_object_id_t rg_object_id, 
                                        fbe_u32_t state, 
                                        fbe_u32_t substate,
                                        fbe_u32_t check_type,
                                        fbe_u32_t action,
                                        fbe_u64_t val1,
                                        fbe_u64_t val2,
                                        fbe_scheduler_debug_hook_t *hook_p) 
{
    fbe_status_t                    status = FBE_STATUS_OK;
    
    hook_p->monitor_state = state;
    hook_p->monitor_substate = substate;
    hook_p->object_id = rg_object_id;
    hook_p->check_type = check_type;
    hook_p->action = action;
    hook_p->val1 = val1;
    hook_p->val2 = val2;
    hook_p->counter = 0;

    status = fbe_api_scheduler_get_debug_hook(hook_p);

    return status;
}
/******************************************
 * end fbe_test_get_debug_hook()
 ******************************************/

/*!***************************************************************************
 *          fbe_test_get_debug_hook_active()
 *****************************************************************************
 *  
 *  @brief  Delete the debug hook for the object specified on the active (for the
 *          object SP). 
 *
 * @param   object_id - object id to wait for
 * @param   state - the state in the monitor 
 * @param   substate - the substate in the monitor  
 * @param   check_type  - check type (state, etc)
 * @param   action - log, pause etc
 * @param   val1 - first 64-bit value ot check
 * @param   val2 - second 64-bit value ot check
 * @param   hook_p - Output information on this hook.
 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  12/05/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_get_debug_hook_active(fbe_object_id_t object_id, 
                                            fbe_u32_t state, 
                                            fbe_u32_t substate,
                                            fbe_u32_t check_type,
                                            fbe_u32_t action,
                                            fbe_u64_t val1,
                                            fbe_u64_t val2,
                                            fbe_scheduler_debug_hook_t *hook_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_metadata_info_t                     metadata_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the active SP for this object.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_base_config_metadata_get_info(object_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the metadata element state for this SP.
     */
    if (metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE)
    {
        active_sp = this_sp;
    }
    else
    {
        /* Else the active is the `other' SP.  Validate that the other SP
         * is alive.
         */
        active_sp = other_sp;
        status = fbe_api_cmi_service_get_info(&cmi_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (cmi_info.peer_alive == FBE_FALSE)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s for obj: 0x%x active SP: %d not alive", 
                       __FUNCTION__, object_id, active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    }

    /* Get the debug hook on the active.
     */
    if (active_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    hook_p->monitor_state = state;
    hook_p->monitor_substate = substate;
    hook_p->object_id = object_id;
    hook_p->check_type = check_type;
    hook_p->action = action;
    hook_p->val1 = val1;
    hook_p->val2 = val2;
    hook_p->counter = 0;
    status = fbe_api_scheduler_get_debug_hook(hook_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we changed SP change it back.
     */
    if (active_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Log a message.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "get hook active: %d obj: 0x%x state: %d substate: %d", 
               active_sp, object_id, state, substate);

    return status;
}
/**************************************
 * end fbe_test_get_debug_hook_active()
 **************************************/

/*!***************************************************************************
 *          fbe_test_get_debug_hook_passive()
 *****************************************************************************
 *  
 *  @brief  Get the debug hook for the object specified on the passive 
 *          (for the object SP). 
 *
 * @param   object_id - object id to wait for
 * @param   state - the state in the monitor 
 * @param   substate - the substate in the monitor  
 * @param   check_type  - check type (state, etc)
 * @param   action - log, pause etc
 * @param   val1 - first 64-bit value ot check
 * @param   val2 - second 64-bit value ot check
 * @param   hook_p - Output information on this hook.
 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  12/05/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_get_debug_hook_passive(fbe_object_id_t object_id, 
                                            fbe_u32_t state, 
                                            fbe_u32_t substate,
                                            fbe_u32_t check_type,
                                            fbe_u32_t action,
                                            fbe_u64_t val1,
                                            fbe_u64_t val2,
                                            fbe_scheduler_debug_hook_t *hook_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_metadata_info_t                     metadata_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   passive_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the active SP for this object.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_base_config_metadata_get_info(object_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the metadata element state for this SP.
     */
    if (metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE)
    {
        passive_sp = this_sp;
    }
    else
    {
        /* Else the passive is the `other' SP.  Validate that the other SP
         * is alive.
         */
        passive_sp = other_sp;
        status = fbe_api_cmi_service_get_info(&cmi_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (cmi_info.peer_alive == FBE_FALSE)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s for obj: 0x%x passive SP: %d not alive", 
                       __FUNCTION__, object_id, passive_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    }

    /* Add the debug hook on the passive.
     */
    if (passive_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(passive_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    hook_p->monitor_state = state;
    hook_p->monitor_substate = substate;
    hook_p->object_id = object_id;
    hook_p->check_type = check_type;
    hook_p->action = action;
    hook_p->val1 = val1;
    hook_p->val2 = val2;
    hook_p->counter = 0;
    status = fbe_api_scheduler_get_debug_hook(hook_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we changed SP change it back.
     */
    if (passive_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Log a message.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "get hook passive: %d obj: 0x%x state: %d substate: %d", 
               passive_sp, object_id, state, substate);

    return status;
}
/***************************************
 * end fbe_test_get_debug_hook_passive()
 ***************************************/

/*!***************************************************************************
 *          fbe_test_del_debug_hook_active()
 *****************************************************************************
 *  
 *  @brief  Delete the debug hook for the object specified on the active (for the
 *          object SP). 
 *
 * @param   object_id - object id to wait for
 * @param   state - the state in the monitor 
 * @param   substate - the substate in the monitor 
 * @param   val1 - first 64-bit value ot check
 * @param   val2 - second 64-bit value ot check 
 * @param   check_type  - check type (state, etc)
 * @param   action - log, pause etc

 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  12/05/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_del_debug_hook_active(fbe_object_id_t object_id, 
                                            fbe_u32_t state, 
                                            fbe_u32_t substate,
                                            fbe_u64_t val1,
                                            fbe_u64_t val2,
                                            fbe_u32_t check_type,
                                            fbe_u32_t action)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_metadata_info_t                     metadata_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the active SP for this object.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_base_config_metadata_get_info(object_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the metadata element state for this SP.
     */
    if (metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE)
    {
        active_sp = this_sp;
    }
    else
    {
        /* Else the active is the `other' SP.  Validate that the other SP
         * is alive.
         */
        active_sp = other_sp;
        status = fbe_api_cmi_service_get_info(&cmi_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (cmi_info.peer_alive == FBE_FALSE)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s for obj: 0x%x active SP: %d not alive", 
                       __FUNCTION__, object_id, active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    }

    /* Add the debug hook on the active.
     */
    if (active_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    status = fbe_api_scheduler_del_debug_hook(object_id,
                                              state,
                                              substate,
                                              val1,
                                              val2,
                                              check_type,
                                              action);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we changed SP change it back.
     */
    if (active_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Log a message.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "del hook active: %d obj: 0x%x state: %d substate: %d", 
               active_sp, object_id, state, substate);

    return status;
}
/**************************************
 * end fbe_test_del_debug_hook_active()
 **************************************/

/*!***************************************************************************
 *          fbe_test_del_debug_hook_passive()
 *****************************************************************************
 *  
 *  @brief  Delete the debug hook for the object specified on the passive 
 *          (for the object SP). 
 *
 * @param   object_id - object id to wait for
 * @param   state - the state in the monitor 
 * @param   substate - the substate in the monitor  
 * @param   val1 - first 64-bit value ot check
 * @param   val2 - second 64-bit value ot check
 * @param   check_type  - check type (state, etc)
 * @param   action - log, pause etc
 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  12/05/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_del_debug_hook_passive(fbe_object_id_t object_id, 
                                            fbe_u32_t state, 
                                            fbe_u32_t substate,
                                            fbe_u64_t val1,
                                            fbe_u64_t val2,
                                            fbe_u32_t check_type,
                                            fbe_u32_t action)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_metadata_info_t                     metadata_info;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   passive_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the active SP for this object.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_base_config_metadata_get_info(object_id, &metadata_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Get the metadata element state for this SP.
     */
    if (metadata_info.metadata_element_state == FBE_METADATA_ELEMENT_STATE_PASSIVE)
    {
        passive_sp = this_sp;
    }
    else
    {
        /* Else the passive is the `other' SP.  Validate that the other SP
         * is alive.
         */
        passive_sp = other_sp;
        status = fbe_api_cmi_service_get_info(&cmi_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (cmi_info.peer_alive == FBE_FALSE)
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s for obj: 0x%x passive SP: %d not alive", 
                       __FUNCTION__, object_id, passive_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    }

    /* Add the debug hook on the passive.
     */
    if (passive_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(passive_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    status = fbe_api_scheduler_del_debug_hook(object_id,
                                              state,
                                              substate,
                                              val1,
                                              val2,
                                              check_type,
                                              action);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we changed SP change it back.
     */
    if (passive_sp != this_sp)
    {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Log a message.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "del hook passive: %d obj: 0x%x state: %d substate: %d", 
               passive_sp, object_id, state, substate);

    return status;
}
/***************************************
 * end fbe_test_del_debug_hook_passive()
 ***************************************/

/*!**************************************************************
* fbe_test_wait_for_hook_counter()
****************************************************************
* @brief
*  Wait for the hook to be hit and the counter to match what the
*  client wants .
*
* @param object_id - object id to wait for
*        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
* @return fbe_status_t - status 
 *
* @author
*  05/11/2012 - Created. Ashok Tamilarasan
*
****************************************************************/
fbe_status_t fbe_test_wait_for_hook_counter(fbe_object_id_t object_id, 
                                            fbe_u32_t state, 
                                            fbe_u32_t substate,
                                            fbe_u32_t check_type,
                                            fbe_u32_t action,
                                            fbe_u64_t val1,
                                            fbe_u64_t val2,
                                            fbe_u64_t counter)
{
    fbe_scheduler_debug_hook_t      hook;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       current_time = 0;
    fbe_u32_t                       timeout_ms = FBE_TEST_HOOK_WAIT_MSEC;
    
    hook.monitor_state = state;
    hook.monitor_substate = substate;
    hook.object_id = object_id;
    hook.check_type = check_type;
    hook.action = action;
    hook.val1 = val1;
    hook.val2 = val2;
    hook.counter = 0;

    mut_printf(MUT_LOG_TEST_STATUS, "Wait for the hook counter:%llu objid: 0x%x state: %d substate: %d.", 
               counter, object_id, state, substate);

    while (current_time < timeout_ms){

        status = fbe_api_scheduler_get_debug_hook(&hook);

        if (hook.counter == counter) {
            mut_printf(MUT_LOG_TEST_STATUS, "Hit the debug hook and counter:%llu matches objid:0x%x state:%d substate:%d.", 
                       counter, object_id, state, substate);
            return status;
        }
        current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "object 0x%x did not hit counter:%d actual:%d state %d substate %d in %d ms!\n", 
                  object_id, (int)counter, (int)hook.counter, state, substate, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
* end fbe_test_wait_for_hook_counter()
******************************************/

/*!***************************************************************************
 *          fbe_test_clear_all_debug_hooks_both_sps()
 *****************************************************************************
 *  
 *  @brief  Delete all the debug hooks for both SPs.
 *
 * @param   None
 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  12/05/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_clear_all_debug_hooks_both_sps(void)
{
    fbe_status_t                            status;
    fbe_scheduler_debug_hook_t              hook;
    fbe_bool_t                              b_is_dualsp_test = FBE_FALSE;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the active SP for this object.
     */
    b_is_dualsp_test = fbe_test_sep_util_get_dualsp_test_mode();
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* First clear all hooks on this SP.
     */
    status = fbe_api_scheduler_clear_all_debug_hooks(&hook);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If dualsp test mode is enabled, check the status of the peer and
     * if it is alive remove all the hooks there also.
     */
    if (b_is_dualsp_test == FBE_TRUE)
    {
        status = fbe_api_cmi_service_get_info(&cmi_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (cmi_info.peer_alive == FBE_FALSE)
        {
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s for peer SP: %d not alive", 
                       __FUNCTION__, other_sp);
            return status;
        }

        /* Else change the active and clear all the hooks on the peer.
         */
        status = fbe_api_sim_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Clear all the hooks
         */
        status = fbe_api_scheduler_clear_all_debug_hooks(&hook);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set the target server back.
         */
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return status;

}
/*********************************************************
 * end fbe_api_scheduler_clear_all_debug_hooks_both_sps()
 *********************************************************/

/*!**************************************************************
 * fbe_test_validate_sniff_progress()
 ****************************************************************
 * @brief
 *  Validate that sniff is running.
 *
 * @param rg_object_id - the rg object id to wait for
 *        state - the state in the monitor 
 *        substate - the substate in the monitor           
 *
 * @return fbe_status_t - status 
 *
 * @author
 *  3/29/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_validate_sniff_progress(fbe_object_id_t pvd_object_id) 
{

    fbe_status_t status;

    /* Wait until we hit the hook.
     */
    status = fbe_test_wait_for_debug_hook_active(pvd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                                                 FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_SEND_VERIFY,
                                                 SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

}
/**************************************
 * end fbe_test_validate_sniff_progress()
 **************************************/

/*!**************************************************************
 * fbe_test_sniff_add_hooks()
 ****************************************************************
 * @brief
 *  Add hooks used to track sniff progress.
 *
 * @param pvd_object_id            
 *
 * @return None
 *
 * @author
 *  4/3/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sniff_add_hooks(fbe_object_id_t pvd_object_id)
{
    fbe_status_t status;

    /* Wait until PVD does not get permission.
     */
    status = fbe_test_add_debug_hook_active(pvd_object_id,
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                                            FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_NO_PERMISSION,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add hook for when we do sniff.  We should not hit this.
     */
    status = fbe_test_add_debug_hook_active(pvd_object_id,
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                                            FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_SEND_VERIFY,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end fbe_test_sniff_add_hooks()
 ******************************************/

/*!**************************************************************
 * fbe_test_sniff_del_hooks()
 ****************************************************************
 * @brief
 *  Remove hooks used to track sniff progress.
 *
 * @param pvd_object_id            
 *
 * @return None
 *
 * @author
 *  4/3/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sniff_del_hooks(fbe_object_id_t pvd_object_id)
{
    fbe_status_t status;

    /* Wait until PVD does not get permission.
     */
    status = fbe_test_del_debug_hook_active(pvd_object_id,
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                                            FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_NO_PERMISSION,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add hook for when we do sniff.  We should not hit this.
     */
    status = fbe_test_del_debug_hook_active(pvd_object_id,
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                                            FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_SEND_VERIFY,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end fbe_test_sniff_del_hooks()
 ******************************************/

/*!**************************************************************
 * fbe_test_sniff_validate_no_progress()
 ****************************************************************
 * @brief
 *  Make sure sniff does not progress.  
 *  Assumes that fbe_test_sniff_add_hooks() was called.
 *
 * @param pvd_object_id            
 *
 * @return None
 *
 * @author
 *  3/29/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_sniff_validate_no_progress(fbe_object_id_t pvd_object_id)
{
    fbe_status_t status;
    fbe_scheduler_debug_hook_t hook;

    /* Wait until PVD does not get permission.
     */
    status = fbe_test_wait_for_debug_hook_active(pvd_object_id, 
                                                 SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                                                 FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_NO_PERMISSION,
                                                 SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                                 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Make sure we did not hit the hook for verify to make progess.
     */
    fbe_test_get_debug_hook_active(pvd_object_id,
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY,
                                   FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_SEND_VERIFY, 
                                   SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                   0,0, &hook);
    if (hook.counter != 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "hook counter is %lld", hook.counter);
        MUT_FAIL_MSG("hook counter is not zero.");
    }
}
/******************************************
 * end fbe_test_sniff_validate_no_progress()
 ******************************************/

/*!**************************************************************
 * fbe_test_zero_add_hooks()
 ****************************************************************
 * @brief
 *  Remove the hooks used to track zero progress.
 *
 * @param pvd_object_id            
 *
 * @return None
 *
 * @author
 *  4/3/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_zero_add_hooks(fbe_object_id_t pvd_object_id)
{
    fbe_status_t status;

    /* Wait until PVD does not get permission.
     */
    status = fbe_test_add_debug_hook_active(pvd_object_id,
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                            FBE_PROVISION_DRIVE_SUBSTATE_ZERO_NO_PERMISSION,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add hook for when we do sniff.  We should not hit this.
     */
    status = fbe_test_add_debug_hook_active(pvd_object_id,
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                            FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end fbe_test_zero_add_hooks()
 ******************************************/
/*!**************************************************************
 * fbe_test_zero_del_hooks()
 ****************************************************************
 * @brief
 *  Remove the hooks used to track zero progress.
 *
 * @param pvd_object_id            
 *
 * @return None
 *
 * @author
 *  4/3/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_zero_del_hooks(fbe_object_id_t pvd_object_id)
{
    fbe_status_t status;

    /* Wait until PVD does not get permission.
     */
    status = fbe_test_del_debug_hook_active(pvd_object_id,
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                            FBE_PROVISION_DRIVE_SUBSTATE_ZERO_NO_PERMISSION,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add hook for when we do sniff.  We should not hit this.
     */
    status = fbe_test_del_debug_hook_active(pvd_object_id,
                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                            FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end fbe_test_zero_del_hooks()
 ******************************************/

/*!**************************************************************
 * fbe_test_zero_validate_no_progress()
 ****************************************************************
 * @brief
 *  Make sure zeroing does not progress.  
 *  Assumes that fbe_test_zero_add_hooks() was called.
 *
 * @param pvd_object_id             
 *
 * @return None.  
 *
 * @author
 *  3/29/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_zero_validate_no_progress(fbe_object_id_t pvd_object_id)
{
    fbe_status_t status;
    fbe_scheduler_debug_hook_t hook;

    /* Wait until PVD does not get permission.
     */
    status = fbe_test_wait_for_debug_hook_active(pvd_object_id, 
                                         SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                                         FBE_PROVISION_DRIVE_SUBSTATE_ZERO_NO_PERMISSION,
                                         SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                         0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Make sure we did not hit the hook for verify to make progess.
     */
    fbe_test_get_debug_hook_active(pvd_object_id,
                               SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO,
                               FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND,
                               SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                               0,0, &hook);
    if (hook.counter != 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "hook counter is %lld", hook.counter);
        MUT_FAIL_MSG("hook counter is not zero.");
    }
}
/******************************************
 * end fbe_test_zero_validate_no_progress()
 ******************************************/

/*!**************************************************************
 * fbe_test_copy_add_hooks()
 ****************************************************************
 * @brief
 *  Remove the hooks used to track copy progress.
 *
 * @param vd_object_id            
 *
 * @return None
 *
 * @author
 *  4/6/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_copy_add_hooks(fbe_object_id_t vd_object_id)
{
    fbe_status_t status;

    /* We will wait until VD does not get permission.
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                            FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_UPSTREAM_PERMISSION,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add hook for when we do copy.  We should not hit this.
     */
    status = fbe_test_add_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                            FBE_RAID_GROUP_SUBSTATE_REBUILD_SEND,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end fbe_test_copy_add_hooks()
 ******************************************/

/*!**************************************************************
 * fbe_test_copy_del_hooks()
 ****************************************************************
 * @brief
 *  Remove the hooks used to track copy progress.
 *
 * @param vd_object_id            
 *
 * @return None
 *
 * @author
 *  4/6/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_copy_del_hooks(fbe_object_id_t vd_object_id)
{
    fbe_status_t status;

    /* Wait until PVD does not get permission.
     */
    status = fbe_test_del_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                            FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_UPSTREAM_PERMISSION,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Add hook for when we do sniff.  We should not hit this.
     */
    status = fbe_test_del_debug_hook_active(vd_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                            FBE_RAID_GROUP_SUBSTATE_REBUILD_SEND,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/******************************************
 * end fbe_test_copy_del_hooks()
 ******************************************/

/*!**************************************************************
 * fbe_test_copy_validate_no_progress()
 ****************************************************************
 * @brief
 *  Make sure copying does not progress.  
 *  Assumes that fbe_test_copy_add_hooks() was called.
 *
 * @param vd_object_id - object id of vd to validate.             
 *
 * @return None.  
 *
 * @author
 *  4/6/2012 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_test_copy_validate_no_progress(fbe_object_id_t vd_object_id)
{
    fbe_status_t status;
    fbe_scheduler_debug_hook_t hook;

    /* Wait until VD does not get permission.
     */
    status = fbe_test_wait_for_debug_hook_active(vd_object_id, 
                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                         FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_UPSTREAM_PERMISSION,
                                         SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                                         0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    /* Make sure we did not hit the hook for copy to make progess.
     */
    fbe_test_get_debug_hook_active(vd_object_id,
                               SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                               FBE_RAID_GROUP_SUBSTATE_REBUILD_SEND,
                               SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                               0,0, &hook);
    if (hook.counter != 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "copy hook counter is %lld", hook.counter);
        MUT_FAIL_MSG("copy hook counter is not zero.");
    }
}
/******************************************
 * end fbe_test_copy_validate_no_progress()
 ******************************************/

/*!**************************************************************
 * fbe_test_verify_validate_no_progress()
 ****************************************************************
 * @brief
 *  Check if background verify did not make progress.
 *
 * @param rg_object_id               
 * @param verify_type
 *
 * @return None
 *
 * @author
 *  3/29/2012 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_test_verify_validate_no_progress(fbe_object_id_t rg_object_id,
                                               fbe_lun_verify_type_t verify_type)
{
    fbe_status_t status;
    fbe_api_raid_group_get_info_t prev_rg_info;
    fbe_api_raid_group_get_info_t rg_info;
    fbe_lba_t prev_chkpt;
    fbe_scheduler_debug_hook_t hook;
    fbe_u32_t monitor_substate;
    fbe_lba_t chkpt;

    /* Wait until Verify does not get permission.
     */
    status = fbe_test_add_debug_hook_active(rg_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            FBE_RAID_GROUP_SUBSTATE_VERIFY_NO_PERMISSION,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_wait_for_debug_hook_active(rg_object_id, 
                                         SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                         FBE_RAID_GROUP_SUBSTATE_VERIFY_NO_PERMISSION,
                                         SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG, 0,0);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* See what the checkpoint is.  Save checkpoint.  This should no longer change.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &prev_rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_api_raid_group_get_verify_checkpoint(&prev_rg_info, verify_type, &prev_chkpt);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    fbe_test_verify_get_verify_start_substate(verify_type, &monitor_substate);

    /* Add hook for when we issue the BV.  We should not hit this.
     */
    status = fbe_test_add_debug_hook_active(rg_object_id, 
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            monitor_substate,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Make sure we did not make progress.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &rg_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    fbe_api_raid_group_get_verify_checkpoint(&rg_info, verify_type, &chkpt);
    if (chkpt != prev_chkpt)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "current verify chkpt 0x%llx != prev chkpt 0x%llx",
                   chkpt, prev_chkpt);
        MUT_FAIL_MSG("verify checkpoint moved unexpectedly");
    }
    /* Make sure we did not hit the hook for verify to make progess.
     */
    fbe_test_get_debug_hook_active(rg_object_id, 
                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                            monitor_substate,
                            SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG,
                            0,0, &hook);
    if (hook.counter != 0)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "hook counter is %lld", hook.counter);
        MUT_FAIL_MSG("hook counter is not zero.");
    }

    /* Remove hooks.
     */
    status = fbe_test_del_debug_hook_active(rg_object_id,
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            FBE_RAID_GROUP_SUBSTATE_VERIFY_NO_PERMISSION,
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    status = fbe_test_del_debug_hook_active(rg_object_id, 
                                            SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY,
                                            monitor_substate, 
                                            0, 0, SCHEDULER_CHECK_STATES, SCHEDULER_DEBUG_ACTION_LOG);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
}
/*******************************************
 * end fbe_test_verify_validate_no_progress()
 *******************************************/


/*!***************************************************************************
 *          fbe_test_use_rg_hooks
 *****************************************************************************
 * @brief
 *  Use a hook by taking the specified action for the specified raid group.
 *
 * @param   rg_config_p - Raid group config
 * @param   num_ds_objects - > 1 if we should use ds object hooks.
 *                           FBE_U32_MAX if we should only use top level.
 * @param   state - the state in the monitor 
 * @param   substate - the substate in the monitor  
 * @param   val1 - first 64-bit value to check
 * @param   val2 - second 64-bit value ot check
 * @param   check_type  - check type (state, etc)
 * @param   action - log, pause etc
 * @param   hook_action - The type of action to take for this raid group,
 *                        e.g. wait, add, delete
 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  11/21/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_test_use_rg_hooks(fbe_test_rg_configuration_t * const rg_config_p, 
                                   fbe_u32_t num_ds_objects, 
                                   fbe_u32_t state, 
                                   fbe_u32_t substate,
                                   fbe_u64_t val1,
                                   fbe_u64_t val2,
                                   fbe_u32_t check_type,
                                   fbe_u32_t action,
                                   fbe_test_hook_action_t hook_action)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_object_id_t rg_object_id;
    fbe_sim_transport_connection_target_t  active_sp;
    fbe_sim_transport_connection_target_t  passive_sp;
    fbe_sim_transport_connection_target_t   this_sp;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();

    status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if ((rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) &&
        (num_ds_objects != FBE_U32_MAX)) {
        fbe_u32_t index;
        fbe_api_base_config_downstream_object_list_t downstream_object_list;
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

        /* For each mirrored pair wait for the state to change..
         */
        for (index = 0; 
              index < FBE_MIN(downstream_object_list.number_of_downstream_objects, num_ds_objects); 
              index++) {

            if ((hook_action != FBE_TEST_HOOK_ACTION_ADD_CURRENT) &&
                (hook_action != FBE_TEST_HOOK_ACTION_WAIT_CURRENT) &&
                (hook_action != FBE_TEST_HOOK_ACTION_DELETE_CURRENT))  {
                fbe_test_sep_util_get_active_passive_sp(downstream_object_list.downstream_object_list[index], &active_sp, &passive_sp);
                fbe_api_sim_transport_set_target_server(active_sp);
            }
            if (hook_action == FBE_TEST_HOOK_ACTION_ADD) {
                status = fbe_test_add_debug_hook_active(downstream_object_list.downstream_object_list[index],
                                                    state, substate, val1, val2, check_type, action);
            }
            else if ((hook_action == FBE_TEST_HOOK_ACTION_WAIT) || 
                     (hook_action == FBE_TEST_HOOK_ACTION_WAIT_CURRENT)) {
                status = fbe_test_wait_for_debug_hook(downstream_object_list.downstream_object_list[index],
                                                      state, substate, check_type, action, val1, val2);
            } 
            else if (hook_action == FBE_TEST_HOOK_ACTION_DELETE) {
                status = fbe_test_del_debug_hook_active(downstream_object_list.downstream_object_list[index],
                                                    state, substate, val1, val2, check_type, action);
            }else if (hook_action == FBE_TEST_HOOK_ACTION_ADD_CURRENT) {
                status = fbe_api_scheduler_add_debug_hook(downstream_object_list.downstream_object_list[index],
                                                          state, substate, val1, val2, check_type, action);
            } else if (hook_action == FBE_TEST_HOOK_ACTION_DELETE_CURRENT) {
                status = fbe_api_scheduler_del_debug_hook(downstream_object_list.downstream_object_list[index],
                                                          state, substate, val1, val2, check_type, action);
            } else if (hook_action == FBE_TEST_HOOK_ACTION_CHECK_NOT_HIT) {
                fbe_scheduler_debug_hook_t hook;
                status = fbe_test_get_debug_hook(downstream_object_list.downstream_object_list[index],
                                                 state, substate, check_type, action, val1, val2,
                                                 &hook);
                if (hook.counter != 0) {
                    mut_printf(MUT_LOG_TEST_STATUS, "Hook counter is 0x%llx not 0", hook.counter);
                    MUT_FAIL_MSG("Hook hit unexpectedly");
                }
            } else {
                MUT_FAIL_MSG("Unexpected hook action.");
            }
            if (status != FBE_STATUS_OK) {
                fbe_api_sim_transport_set_target_server(this_sp);
                return status;
            }
        }    /* end for each mirrored pair. */
    } else {
        if ((hook_action != FBE_TEST_HOOK_ACTION_ADD_CURRENT) &&
            (hook_action != FBE_TEST_HOOK_ACTION_WAIT_CURRENT) &&
            (hook_action != FBE_TEST_HOOK_ACTION_DELETE_CURRENT)){
            fbe_test_sep_util_get_active_passive_sp(rg_object_id, &active_sp, &passive_sp);
            fbe_api_sim_transport_set_target_server(active_sp);
        }
        if (hook_action == FBE_TEST_HOOK_ACTION_ADD) {
            status = fbe_test_add_debug_hook_active(rg_object_id,
                                                    state, substate, val1, val2, check_type, action);
        } else if ((hook_action == FBE_TEST_HOOK_ACTION_WAIT) || 
                   (hook_action == FBE_TEST_HOOK_ACTION_WAIT_CURRENT)) {
            status = fbe_test_wait_for_debug_hook(rg_object_id,
                                                  state, substate, check_type, action, val1, val2);
        } else if (hook_action == FBE_TEST_HOOK_ACTION_DELETE) {
            status = fbe_test_del_debug_hook_active(rg_object_id,
                                                    state, substate, val1, val2, check_type, action);
        } else if (hook_action == FBE_TEST_HOOK_ACTION_ADD_CURRENT) {
            status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                                      state, substate, val1, val2, check_type, action);
        } else if (hook_action == FBE_TEST_HOOK_ACTION_DELETE_CURRENT) {
            status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                                      state, substate, val1, val2, check_type, action);
        } else if (hook_action == FBE_TEST_HOOK_ACTION_CHECK_NOT_HIT) {
            fbe_scheduler_debug_hook_t hook;
            status = fbe_test_get_debug_hook(rg_object_id,
                                             state, substate, check_type, action, val1, val2,
                                             &hook);
            if (hook.counter != 0) {
                mut_printf(MUT_LOG_TEST_STATUS, "Hook counter is 0x%llx not 0", hook.counter);
                MUT_FAIL_MSG("Hook hit unexpectedly");
            }
        } else {
            MUT_FAIL_MSG("Unexpected hook action.");
        }
        fbe_api_sim_transport_set_target_server(this_sp);
        return status;
    }
    fbe_api_sim_transport_set_target_server(this_sp);
    return status;
}
/**************************************
 * end fbe_test_use_rg_hooks()
 **************************************/

/*!***************************************************************************
 *          fbe_test_use_rg_hooks_passive()
 *****************************************************************************
 * @brief
 *  Use a hook by taking the specified action for the specified raid group.
 *
 * @param   rg_config_p - Raid group config
 * @param   num_ds_objects - > 1 if we should use ds object hooks.
 *                           FBE_U32_MAX if we should only use top level.
 * @param   state - the state in the monitor 
 * @param   substate - the substate in the monitor  
 * @param   val1 - first 64-bit value to check
 * @param   val2 - second 64-bit value ot check
 * @param   check_type  - check type (state, etc)
 * @param   action - log, pause etc
 * @param   hook_action - The type of action to take for this raid group,
 *                        e.g. wait, add, delete
 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  05/12/2015  Ron Proulx - Copied from fbe_test_use_rg_hooks()
 *
 *****************************************************************************/
fbe_status_t fbe_test_use_rg_hooks_passive(fbe_test_rg_configuration_t * const rg_config_p, 
                                   fbe_u32_t num_ds_objects, 
                                   fbe_u32_t state, 
                                   fbe_u32_t substate,
                                   fbe_u64_t val1,
                                   fbe_u64_t val2,
                                   fbe_u32_t check_type,
                                   fbe_u32_t action,
                                   fbe_test_hook_action_t hook_action)
{
    fbe_status_t status;
    fbe_test_rg_configuration_t * current_rg_config_p = rg_config_p;
    fbe_object_id_t rg_object_id;
    fbe_sim_transport_connection_target_t  active_sp;
    fbe_sim_transport_connection_target_t  passive_sp;
    fbe_sim_transport_connection_target_t   this_sp;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();

    status = fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    if ((rg_config_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) &&
        (num_ds_objects != FBE_U32_MAX)) {
        fbe_u32_t index;
        fbe_api_base_config_downstream_object_list_t downstream_object_list;
        fbe_test_sep_util_get_ds_object_list(rg_object_id, &downstream_object_list);

        /* For each mirrored pair wait for the state to change..
         */
        for (index = 0; 
              index < FBE_MIN(downstream_object_list.number_of_downstream_objects, num_ds_objects); 
              index++) {

            /* Execute the hook action on the object's passive SP.
             */
            if ((hook_action != FBE_TEST_HOOK_ACTION_ADD_CURRENT) &&
                (hook_action != FBE_TEST_HOOK_ACTION_WAIT_CURRENT) &&
                (hook_action != FBE_TEST_HOOK_ACTION_DELETE_CURRENT))  {
                fbe_test_sep_util_get_active_passive_sp(downstream_object_list.downstream_object_list[index], &active_sp, &passive_sp);
                fbe_api_sim_transport_set_target_server(passive_sp);
            }
            if (hook_action == FBE_TEST_HOOK_ACTION_ADD) {
                status = fbe_test_add_debug_hook_passive(downstream_object_list.downstream_object_list[index],
                                                         state, substate, val1, val2, check_type, action);
            }
            else if ((hook_action == FBE_TEST_HOOK_ACTION_WAIT) || 
                     (hook_action == FBE_TEST_HOOK_ACTION_WAIT_CURRENT)) {
                status = fbe_test_wait_for_debug_hook(downstream_object_list.downstream_object_list[index],
                                                      state, substate, check_type, action, val1, val2);
            } 
            else if (hook_action == FBE_TEST_HOOK_ACTION_DELETE) {
                status = fbe_test_del_debug_hook_passive(downstream_object_list.downstream_object_list[index],
                                                    state, substate, val1, val2, check_type, action);
            }else if (hook_action == FBE_TEST_HOOK_ACTION_ADD_CURRENT) {
                status = fbe_api_scheduler_add_debug_hook(downstream_object_list.downstream_object_list[index],
                                                          state, substate, val1, val2, check_type, action);
            } else if (hook_action == FBE_TEST_HOOK_ACTION_DELETE_CURRENT) {
                status = fbe_api_scheduler_del_debug_hook(downstream_object_list.downstream_object_list[index],
                                                          state, substate, val1, val2, check_type, action);
            } else if (hook_action == FBE_TEST_HOOK_ACTION_CHECK_NOT_HIT) {
                fbe_scheduler_debug_hook_t hook;
                status = fbe_test_get_debug_hook(downstream_object_list.downstream_object_list[index],
                                                 state, substate, check_type, action, val1, val2,
                                                 &hook);
                if (hook.counter != 0) {
                    mut_printf(MUT_LOG_TEST_STATUS, "Hook counter is %lld not 0", hook.counter);
                    MUT_FAIL_MSG("Hook hit unexpectedly");
                }
            } else {
                MUT_FAIL_MSG("Unexpected hook action.");
            }
            if (status != FBE_STATUS_OK) {
                fbe_api_sim_transport_set_target_server(this_sp);
                return status;
            }
        }    /* end for each mirrored pair. */
    } else {
        if ((hook_action != FBE_TEST_HOOK_ACTION_ADD_CURRENT) &&
            (hook_action != FBE_TEST_HOOK_ACTION_WAIT_CURRENT) &&
            (hook_action != FBE_TEST_HOOK_ACTION_DELETE_CURRENT)){
            fbe_test_sep_util_get_active_passive_sp(rg_object_id, &active_sp, &passive_sp);
            fbe_api_sim_transport_set_target_server(passive_sp);
        }
        if (hook_action == FBE_TEST_HOOK_ACTION_ADD) {
            status = fbe_test_add_debug_hook_passive(rg_object_id,
                                                    state, substate, val1, val2, check_type, action);
        } else if ((hook_action == FBE_TEST_HOOK_ACTION_WAIT) || 
                   (hook_action == FBE_TEST_HOOK_ACTION_WAIT_CURRENT)) {
            status = fbe_test_wait_for_debug_hook(rg_object_id,
                                                  state, substate, check_type, action, val1, val2);
        } else if (hook_action == FBE_TEST_HOOK_ACTION_DELETE) {
            status = fbe_test_del_debug_hook_passive(rg_object_id,
                                                    state, substate, val1, val2, check_type, action);
        } else if (hook_action == FBE_TEST_HOOK_ACTION_ADD_CURRENT) {
            status = fbe_api_scheduler_add_debug_hook(rg_object_id,
                                                      state, substate, val1, val2, check_type, action);
        } else if (hook_action == FBE_TEST_HOOK_ACTION_DELETE_CURRENT) {
            status = fbe_api_scheduler_del_debug_hook(rg_object_id,
                                                      state, substate, val1, val2, check_type, action);
        } else if (hook_action == FBE_TEST_HOOK_ACTION_CHECK_NOT_HIT) {
            fbe_scheduler_debug_hook_t hook;
            status = fbe_test_get_debug_hook(rg_object_id,
                                             state, substate, check_type, action, val1, val2,
                                             &hook);
            if (hook.counter != 0) {
                mut_printf(MUT_LOG_TEST_STATUS, "Hook counter is %lld not 0", hook.counter);
                MUT_FAIL_MSG("Hook hit unexpectedly");
            }
        } else {
            MUT_FAIL_MSG("Unexpected hook action.");
        }
        fbe_api_sim_transport_set_target_server(this_sp);
        return status;
    }
    fbe_api_sim_transport_set_target_server(this_sp);
    return status;
}
/**************************************
 * end fbe_test_use_rg_hooks_passive()
 **************************************/

/*!***************************************************************************
 *          fbe_test_rg_config_use_rg_hooks_both_sps()
 *****************************************************************************
 * @brief
 *  Use a hook by taking the specified action for the specified raid groups.
 *
 * @param   rg_config_p - Raid group config array
 * @param   raid_group_count - Number of raid group configs in array
 * @param   num_ds_objects - > 1 if we should use ds object hooks.
 *                           FBE_U32_MAX if we should only use top level.
 * @param   state - the state in the monitor 
 * @param   substate - the substate in the monitor  
 * @param   val1 - first 64-bit value to check
 * @param   val2 - second 64-bit value ot check
 * @param   check_type  - check type (state, etc)
 * @param   action - log, pause etc
 * @param   hook_action - The type of action to take for this raid group,
 *                        e.g. wait, add, delete
 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  05/12/2015  Ron Proulx - Copied from fbe_test_use_rg_hooks()
 *
 *****************************************************************************/
fbe_status_t fbe_test_rg_config_use_rg_hooks_both_sps(fbe_test_rg_configuration_t * const rg_config_p, 
                                                      fbe_u32_t raid_group_count,
                                                      fbe_u32_t num_ds_objects, 
                                                      fbe_u32_t state, 
                                                      fbe_u32_t substate,
                                                      fbe_u64_t val1,
                                                      fbe_u64_t val2,
                                                      fbe_u32_t check_type,
                                                      fbe_u32_t action,
                                                      fbe_test_hook_action_t hook_action)
{
    fbe_status_t                            status;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_u32_t                               rg_index;
    fbe_bool_t                              b_is_dualsp_test = FBE_FALSE;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_cmi_service_get_info_t              cmi_info;
    fbe_bool_t                              b_peer_alive = FBE_FALSE;

    /* Get the active SP for this object.
     */
    b_is_dualsp_test = fbe_test_sep_util_get_dualsp_test_mode();
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;

    /* If dualsp test mode is enabled, check the status of the peer and
     * if it is alive remove all the hooks there also.
     */
    if (b_is_dualsp_test == FBE_TRUE)
    {
        status = fbe_api_cmi_service_get_info(&cmi_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (cmi_info.peer_alive == FBE_TRUE)
        {
            b_peer_alive = FBE_TRUE;
        }
    }

    /* For all raid group configurations.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* First take the hook action on the Active SP
         */
        status = fbe_test_use_rg_hooks(current_rg_config_p, num_ds_objects, 
                                       state, 
                                       substate,
                                       val1, val2,
                                       check_type,
                                       action,
                                       hook_action);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* If the peer is alive take the hook active on peer also.
         */
        if (b_peer_alive)
        {
            /* Depending on the request we need to use either `use hooks' after
             * changing sp or use hooks passive.
             */
            if ((hook_action != FBE_TEST_HOOK_ACTION_ADD_CURRENT)   &&
                (hook_action != FBE_TEST_HOOK_ACTION_WAIT_CURRENT)  &&
                (hook_action != FBE_TEST_HOOK_ACTION_DELETE_CURRENT)   )  
            {
                /* Change hooks on passive.
                 */
                status = fbe_test_use_rg_hooks_passive(current_rg_config_p, num_ds_objects, 
                                                       state, 
                                                       substate,
                                                       val1, val2,
                                                       check_type,
                                                       action,
                                                       hook_action);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
            else
            {
                /* Else simply switch SP to peer.
                 */
                fbe_api_sim_transport_set_target_server(other_sp);
                status = fbe_test_use_rg_hooks(current_rg_config_p, num_ds_objects, 
                                       state, 
                                       substate,
                                       val1, val2,
                                       check_type,
                                       action,
                                       hook_action);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                fbe_api_sim_transport_set_target_server(this_sp);
            }
        }

        /* Goto next rg config
         */
        current_rg_config_p++;
    }

    return status;
}
/************************************************ 
 * end fbe_test_rg_config_use_rg_hooks_both_sps()
 ************************************************/

/*!***************************************************************************
 *          fbe_test_use_pvd_hooks
 *****************************************************************************
 * @brief
 *  Use a hook by taking the specified action for the specified provision drive
 *  (specified by position) for each raid group specified.
 *
 * @param   rg_config_p - Raid group config
 * @param   state - the state in the monitor 
 * @param   substate - the substate in the monitor  
 * @param   val1 - first 64-bit value to check
 * @param   val2 - second 64-bit value ot check
 * @param   check_type  - check type (state, etc)
 * @param   action - log, pause etc
 * @param   hook_action - The type of action to take for this raid group,
 *                        e.g. wait, add, delete
 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  11/21/2013 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_test_use_pvd_hooks(fbe_test_rg_configuration_t * const rg_config_p, 
                                   fbe_u32_t position, /* Take the hook action for the pvd associated with this position*/
                                   fbe_u32_t state, 
                                   fbe_u32_t substate,
                                   fbe_u64_t val1,
                                   fbe_u64_t val2,
                                   fbe_u32_t check_type,
                                   fbe_u32_t action,
                                   fbe_test_hook_action_t hook_action)
{
    fbe_status_t status;
    fbe_object_id_t pvd_object_id;
    fbe_sim_transport_connection_target_t  active_sp;
    fbe_sim_transport_connection_target_t  passive_sp;
    fbe_sim_transport_connection_target_t   this_sp;

    /* Get the local SP ID */
    this_sp = fbe_api_sim_transport_get_target_server();

    /* Get the pvd object id */
    status = fbe_api_provision_drive_get_obj_id_by_location(rg_config_p->rg_disk_set[position].bus,
                                                            rg_config_p->rg_disk_set[position].enclosure,
                                                            rg_config_p->rg_disk_set[position].slot,
                                                            &pvd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, pvd_object_id);

    /* Now add the hook*/

    if ((hook_action != FBE_TEST_HOOK_ACTION_ADD_CURRENT) &&
        (hook_action != FBE_TEST_HOOK_ACTION_WAIT_CURRENT) &&
        (hook_action != FBE_TEST_HOOK_ACTION_DELETE_CURRENT)){
        fbe_test_sep_util_get_active_passive_sp(pvd_object_id, &active_sp, &passive_sp);
        fbe_api_sim_transport_set_target_server(active_sp);
    }
    if (hook_action == FBE_TEST_HOOK_ACTION_ADD) {
        status = fbe_test_add_debug_hook_active(pvd_object_id,
                                                state, substate, val1, val2, check_type, action);
    } else if ((hook_action == FBE_TEST_HOOK_ACTION_WAIT) || 
               (hook_action == FBE_TEST_HOOK_ACTION_WAIT_CURRENT)) {
        status = fbe_test_wait_for_debug_hook(pvd_object_id,
                                              state, substate, check_type, action, val1, val2);
    } else if (hook_action == FBE_TEST_HOOK_ACTION_DELETE) {
        status = fbe_test_del_debug_hook_active(pvd_object_id,
                                                state, substate, val1, val2, check_type, action);
    }
    else if (hook_action == FBE_TEST_HOOK_ACTION_ADD_CURRENT) {
        status = fbe_api_scheduler_add_debug_hook(pvd_object_id,
                                                  state, substate, val1, val2, check_type, action);
    } else if (hook_action == FBE_TEST_HOOK_ACTION_DELETE_CURRENT) {
        status = fbe_api_scheduler_del_debug_hook(pvd_object_id,
                                                  state, substate, val1, val2, check_type, action);
    } else if (hook_action == FBE_TEST_HOOK_ACTION_DELETE_CURRENT) {
        status = fbe_api_scheduler_del_debug_hook(pvd_object_id,
                                                  state, substate, val1, val2, check_type, action);
    }
    fbe_api_sim_transport_set_target_server(this_sp);
    return status;
}
/**************************************
 * end fbe_test_use_pvd_hooks()
 **************************************/

/*!***************************************************************************
 *          fbe_test_add_job_service_hook()
 *****************************************************************************
 *  
 *  @brief  Add the job service hook for the object specified on the active SP.
 *
 * @param   object_id - The object id associated with this hook.
 *              Note FBE_OBJECT_ID_INVALID is not allowed.
 * @param   hook_type - The job type to set the hook for.
 * @param   hook_state - The job action state to set the hook for.
 * @param   hook_phase - The phase in the job state specified to set the hook.
 *              For instance the start of persist or the end of persist etc.
 * @param   hook_action - The action to take when the hook is reached.  For
 *              example PAUSE the job service thread or just log the hook etc.
 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  05/20/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_add_job_service_hook(fbe_object_id_t object_id,                   
                                           fbe_job_type_t hook_type,                    
                                           fbe_job_action_state_t hook_state,           
                                           fbe_job_debug_hook_state_phase_t hook_phase, 
                                           fbe_job_debug_hook_action_t hook_action)     
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the job service SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (cmi_info.sp_state != FBE_CMI_STATE_ACTIVE) {
        active_sp = other_sp;
        if (cmi_info.peer_alive == FBE_FALSE) {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s for obj: 0x%x active SP: %d not alive", 
                       __FUNCTION__, object_id, active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    } else {
        active_sp = this_sp;
    }


    /* Add the debug hook on the active.
     */
    if (active_sp != this_sp) {
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    status = fbe_api_job_service_add_debug_hook(object_id,
                                                hook_type,
                                                hook_state,
                                                hook_phase,
                                                hook_action);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we changed SP change it back.
     */
    if (active_sp != this_sp) {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Log a message.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "add job service hook active: %d obj: 0x%x type: 0x%x state: %d phase: %d action: %d", 
               active_sp, object_id, hook_type, hook_state, hook_phase, hook_action);

    return status;
}
/**************************************
 * end fbe_test_add_job_service_hook()
 **************************************/

/*!***************************************************************************
 *          fbe_test_wait_for_job_service_hook()
 *****************************************************************************
 *  
 *  @brief  Wait for the job service hook for the object specified on the active.
 *
 * @param   object_id - The object id associated with this hook.
 *              Note FBE_OBJECT_ID_INVALID is not allowed.
 * @param   hook_type - The job type to set the hook for.
 * @param   hook_state - The job action state to set the hook for.
 * @param   hook_phase - The phase in the job state specified to set the hook.
 *              For instance the start of persist or the end of persist etc.
 * @param   hook_action - The action to take when the hook is reached.  For
 *              example PAUSE the job service thread or just log the hook etc.
 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  05/19/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_wait_for_job_service_hook(fbe_object_id_t object_id,                     
                                                fbe_job_type_t hook_type,                      
                                                fbe_job_action_state_t hook_state,             
                                                fbe_job_debug_hook_state_phase_t hook_phase,   
                                                fbe_job_debug_hook_action_t hook_action)       
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_cmi_service_get_info_t              cmi_info;
    fbe_job_service_debug_hook_t            job_hook;
    fbe_u32_t                               current_time = 0;
    fbe_u32_t                               timeout_ms = fbe_test_get_timeout_ms();

    /* Get the job service SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (cmi_info.sp_state != FBE_CMI_STATE_ACTIVE) {
        active_sp = other_sp;
        if (cmi_info.peer_alive == FBE_FALSE) {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s for obj: 0x%x active SP: %d not alive", 
                       __FUNCTION__, object_id, active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    } else {
        active_sp = this_sp;
    }

    /* Wait for the job service hook on the CMI active.
     */
    if (active_sp != this_sp) {
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Wait until we hit the hook.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "Wait for job service hook. objid: 0x%x type: 0x%x state: %d.", 
               object_id, hook_type, hook_state);

    while (current_time < timeout_ms){
        status = fbe_api_job_service_get_debug_hook(object_id,
                                                    hook_type,
                                                    hook_state,
                                                    hook_phase,
                                                    hook_action,
                                                    &job_hook);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        if (job_hook.hit_counter != 0) {
            if (active_sp != this_sp) {
                status = fbe_api_sim_transport_set_target_server(this_sp);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "We have hit job service hook objid: 0x%x type: 0x%x state: %d.", 
                       object_id, hook_type, hook_state);
            return status;
        }
        current_time = current_time + FBE_TEST_HOOK_POLLING_MSEC;
        fbe_api_sleep(FBE_TEST_HOOK_POLLING_MSEC);
    }

    if (active_sp != this_sp) {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                  "%s: object 0x%x did not hit job service hook type: 0x%x state: %d in %d ms!\n", 
                  __FUNCTION__, object_id, hook_type, hook_state, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
* end fbe_test_wait_for_job_service_hook()
******************************************/

/*!***************************************************************************
 *          fbe_test_get_job_service_hook()
 *****************************************************************************
 *  
 *  @brief  Get the state of the job service hook specified.
 *
 * @param   object_id - The object id associated with this hook.
 *              Note FBE_OBJECT_ID_INVALID is not allowed.
 * @param   hook_type - The job type to set the hook for.
 * @param   hook_state - The job action state to set the hook for.
 * @param   hook_phase - The phase in the job state specified to set the hook.
 *              For instance the start of persist or the end of persist etc.
 * @param   hook_action - The action to take when the hook is reached.  For
 *              example PAUSE the job service thread or just log the hook etc.
 * @param   hook_p - If FBE_STATUS_OK this hook is populated
 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  05/19/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_get_job_service_hook(fbe_object_id_t object_id,                     
                                           fbe_job_type_t hook_type,                      
                                           fbe_job_action_state_t hook_state,             
                                           fbe_job_debug_hook_state_phase_t hook_phase,   
                                           fbe_job_debug_hook_action_t hook_action,
                                           fbe_job_service_debug_hook_t *hook_p)      
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the job service SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (cmi_info.sp_state != FBE_CMI_STATE_ACTIVE) {
        active_sp = other_sp;
        if (cmi_info.peer_alive == FBE_FALSE) {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s for obj: 0x%x active SP: %d not alive", 
                       __FUNCTION__, object_id, active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    } else {
        active_sp = this_sp;
    }

    /* Get the debug hook on the active.
     */
    if (active_sp != this_sp) {
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    status = fbe_api_job_service_get_debug_hook(object_id,
                                                hook_type,
                                                hook_state,
                                                hook_phase,
                                                hook_action,
                                                hook_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we changed SP change it back.
     */
    if (active_sp != this_sp) {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Log a message.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "get job service hook active: %d obj: 0x%x type: 0x%x state: %d phase: %d action: %d count: %d job num: 0x%llx", 
               active_sp, object_id, hook_type, hook_state, hook_phase, hook_action,
               hook_p->hit_counter, hook_p->job_number);

    return status;
}
/**************************************
 * end fbe_test_get_job_service_hook()
 **************************************/

/*!***************************************************************************
 *          fbe_test_del_job_service_hook()
 *****************************************************************************
 *  
 *  @brief  Delete the debug hook for the object specified on the active (for the
 *          object SP). 
 *
 * @param   object_id - The object id associated with this hook.
 *              Note FBE_OBJECT_ID_INVALID is not allowed.
 * @param   hook_type - The job type to set the hook for.
 * @param   hook_state - The job action state to set the hook for.
 * @param   hook_phase - The phase in the job state specified to set the hook.
 *              For instance the start of persist or the end of persist etc.
 * @param   hook_action - The action to take when the hook is reached.  For
 *              example PAUSE the job service thread or just log the hook etc.

 *
 * @return  fbe_status_t - status 
 *
 * @author
 *  05/19.2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_del_job_service_hook(fbe_object_id_t object_id,                          
                                           fbe_job_type_t hook_type,                    
                                           fbe_job_action_state_t hook_state,           
                                           fbe_job_debug_hook_state_phase_t hook_phase, 
                                           fbe_job_debug_hook_action_t hook_action)     
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   active_sp;
    fbe_cmi_service_get_info_t              cmi_info;

    /* Get the job service SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if (cmi_info.sp_state != FBE_CMI_STATE_ACTIVE) {
        active_sp = other_sp;
        if (cmi_info.peer_alive == FBE_FALSE) {
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s for obj: 0x%x active SP: %d not alive", 
                       __FUNCTION__, object_id, active_sp);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
        }
    } else {
        active_sp = this_sp;
    }

    /* Add the debug hook on the active.
     */
    if (active_sp != this_sp) {
        status = fbe_api_sim_transport_set_target_server(active_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }
    status = fbe_api_job_service_delete_debug_hook(object_id,
                                                   hook_type,
                                                   hook_state,
                                                   hook_phase,
                                                   hook_action);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* If we changed SP change it back.
     */
    if (active_sp != this_sp) {
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Log a message.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "del job service hook active: %d obj: 0x%x type: 0x%x state: %d phase: %d action:%d", 
               active_sp, object_id, hook_type, hook_state, hook_phase, hook_action);

    return status;
}
/**************************************
 * end fbe_test_del_job_service_hook()
 **************************************/

/*!***************************************************************************
 *          fbe_test_add_scheduler_hook
 *****************************************************************************
 *
 * @brief  A simpler wrapper for fbe_api_scheduler_add_debug_hook
 *
 * @param   hook - The debug hook to add
 *
 * @return  fbe_status_t - status
 *
 * @author
 *  10/08.2014  Created. Jamin Kang
 *
 *****************************************************************************/
fbe_status_t fbe_test_add_scheduler_hook(const fbe_scheduler_debug_hook_t *hook)
{

    fbe_status_t status;
    status = fbe_api_scheduler_add_debug_hook(hook->object_id,
                                              hook->monitor_state, hook->monitor_substate,
                                              hook->val1, hook->val2,
                                              hook->check_type, hook->action);
    return status;
}
/**************************************
 * end fbe_test_add_scheduler_hook()
 **************************************/

/*!***************************************************************************
 *          fbe_test_del_scheduler_hook
 *****************************************************************************
 *
 * @brief  A simpler wrapper for fbe_api_scheduler_del_debug_hook
 *
 * @param   hook - The debug hook to add
 *
 * @return  fbe_status_t - status
 *
 * @author
 *  10/08.2014  Created. Jamin Kang
 *
 *****************************************************************************/
fbe_status_t fbe_test_del_scheduler_hook(const fbe_scheduler_debug_hook_t *hook)
{

    fbe_status_t status;
    status = fbe_api_scheduler_del_debug_hook(hook->object_id,
                                              hook->monitor_state, hook->monitor_substate,
                                              hook->val1, hook->val2,
                                              hook->check_type, hook->action);
    return status;
}
/**************************************
 * end fbe_test_del_scheduler_hook()
 **************************************/

/*!***************************************************************************
 *          fbe_test_wait_for_scheduler_hook
 *****************************************************************************
 *
 * @brief  A simpler wrapper for fbe_test_wait_for_debug_hook
 *
 * @param   hook - The debug hook to add
 *
 * @return  fbe_status_t - status
 *
 * @author
 *  10/08.2014  Created. Jamin Kang
 *
 *****************************************************************************/
fbe_status_t fbe_test_wait_for_scheduler_hook(const fbe_scheduler_debug_hook_t *hook)
{

    fbe_status_t status;

    status = fbe_test_wait_for_debug_hook(hook->object_id,
                                          hook->monitor_state, hook->monitor_substate,
                                          hook->check_type, hook->action,
                                          hook->val1, hook->val2);
    return status;
}
/**************************************
 * end fbe_test_wait_for_scheduler_hook()
 **************************************/


/*******************************
 * end file fbe_test_sep_hook.c
 *******************************/
