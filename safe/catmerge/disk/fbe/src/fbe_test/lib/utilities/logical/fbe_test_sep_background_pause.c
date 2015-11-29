/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_test_sep_background_pause.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the methods to start, validate and stop background
 *  operations with a `pause at event X'.
 * 
 * @author
 *  06/21/2012  Ron Proulx  - Created.
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/
#include "sep_tests.h"
#include "fbe_test_common_utils.h"
#include "pp_utils.h"
#include "sep_utils.h"
#include "neit_utils.h"
#include "sep_test_io.h"
#include "sep_test_io_private.h"
#include "sep_test_background_ops.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_event_log_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_protocol_error_injection_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_job_service_interface.h"
#include "fbe/fbe_api_scheduler_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe/fbe_api_cmi_interface.h"

/*****************************************
 * DEFINITIONS
 *****************************************/
#define FBE_TEST_SEP_BACKGROUND_PAUSE_SHORT_RETRY_TIME       50     // wait time in ms, in between retries 
#define FBE_TEST_SEP_BACKGROUND_PAUSE_MAX_SHORT_RETRIES    1200

/*!*******************************************************************
 * @def     FBE_TEST_SEP_BACKGROUND_OPS_INVALID_DISK_POSITION
 *********************************************************************
 *  @brief  Invalid disk position.  Used when searching for a disk 
 *          position and no disk is found that meets the criteria.
 *
 *********************************************************************/
#define FBE_TEST_SEP_BACKGROUND_PAUSE_INVALID_DISK_POSITION           ((fbe_u32_t) -1)

/*************************
 *   GLOBALS
 *************************/

/*!*******************************************************************
 * @var fbe_test_sep_background_pause_wait_for_percent_rebuilt_value
 *********************************************************************
 * @brief This is percent rebuilt value that we are waiting for.
 *
 *********************************************************************/
static fbe_u32_t fbe_test_sep_background_pause_wait_for_percent_rebuilt_value  = 0;  /* By default stop once rebuild starts */

/*!*******************************************************************
 * @var     fbe_test_sep_background_pause_b_is_private_rebuild_hook_set
 *********************************************************************
 * @brief   A bool that simply indicates if we have a `private' rebuild
 *          hook set or not.
 *
 *********************************************************************/
static fbe_bool_t fbe_test_sep_background_pause_b_is_private_rebuild_hook_set   = FBE_FALSE;

/*!*******************************************************************
 * @var     fbe_test_sep_background_pause_b_is_initiate_copy_complete_hook_set
 *********************************************************************
 * @brief   A bool that simply indicates if we have set the `initiate
 *          copy complete' hook or not
 *
 *********************************************************************/
static fbe_bool_t fbe_test_sep_background_pause_b_is_initiate_copy_complete_hook_set = FBE_FALSE;

/*!*******************************************************************
 * @var     fbe_test_sep_background_pause_b_is_rebuild_start_hook_set
 *********************************************************************
 * @brief   A bool that simply indicates if we have have set a rebuild
 *          hook for starting the metadata rebuild or not.
 *
 *********************************************************************/
static fbe_bool_t fbe_test_sep_background_pause_b_is_rebuild_start_hook_set = FBE_FALSE;

/*!*******************************************************************
 * @var     fbe_test_sep_background_pause_b_is_swap_out_complete_hook_set
 *********************************************************************
 * @brief   A bool that simply indicates if we have set the `swap-out
 *          complete' hook or not
 *
 *********************************************************************/
static fbe_bool_t fbe_test_sep_background_pause_b_is_swap_out_complete_hook_set = FBE_FALSE;

/*************************
 *   FUNCTIONS
 *************************/


/*!***************************************************************************
 * fbe_test_sep_background_pause_remove_debug_hook()
 *****************************************************************************
 * @brief
 *  This function removes the debug hook (always from the active SP).
 *
 * @param   debug_hook_p - Debug hook to remove.
 * @param   hook_index - For debug the index into the hook array
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test
 *                             FBE_FALSE - This is not a reboot test
 *
 * @return  status
 *
 * @author
 *  03/26/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_pause_remove_debug_hook(fbe_scheduler_debug_hook_t *debug_hook_p, 
                                                             fbe_u32_t hook_index,
                                                             fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_bool_t                              b_is_dualsp_mode = FBE_FALSE;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_bool_t                              b_remove_from_both = FBE_FALSE;     
    fbe_cmi_service_get_info_t              cmi_info;
  
    /* Validate the hook index.
     */
    if (hook_index >= MAX_SCHEDULER_DEBUG_HOOKS)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Attempt to use index: %d greater than max index: %d",
                   __FUNCTION__, hook_index, (MAX_SCHEDULER_DEBUG_HOOKS -1));
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if ((b_is_dualsp_mode == FBE_TRUE)    &&
        (b_is_reboot_test == FBE_TRUE)    &&
        (cmi_info.peer_alive == FBE_TRUE)    )
    {
        b_remove_from_both = FBE_TRUE;
    }

    /* Validate the hook.
     */
    /* Check if we need to clean the hook specified.
     */
    if ((debug_hook_p->object_id == 0)                     ||
        (debug_hook_p->object_id == FBE_OBJECT_ID_INVALID)    )
    {
        /* We don't expect the hook to be left set.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
               "background remove debug hook: unexpected hook set index: %d obj: 0x%x state: %d substate: %d val1: 0x%llx val2: 0x%llx check type: %d action: %d",
               hook_index, debug_hook_p->object_id, debug_hook_p->monitor_state, debug_hook_p->monitor_substate, 
               (unsigned long long)debug_hook_p->val1, (unsigned long long)debug_hook_p->val2, debug_hook_p->check_type, debug_hook_p->action);

        status = FBE_STATUS_GENERIC_FAILURE;
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Display some information
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "background remove debug hook: debug SP: %d(%d) index: %d obj: 0x%x state: %d substate: %d val1: 0x%llx action: %d",
               this_sp, (b_remove_from_both) ? other_sp : this_sp,
               hook_index, debug_hook_p->object_id, 
               debug_hook_p->monitor_state, debug_hook_p->monitor_substate, (unsigned long long)debug_hook_p->val1, debug_hook_p->action);

    /* Remove the debug hook (which should allow the rebuild to continue)
     */
    status = fbe_api_scheduler_del_debug_hook(debug_hook_p->object_id,
                                              debug_hook_p->monitor_state,
                                              debug_hook_p->monitor_substate,
                                              debug_hook_p->val1,
                                              debug_hook_p->val2,
                                              debug_hook_p->check_type,
                                              debug_hook_p->action);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Determine if we are in dualsp mode or not.  If we are in dualsp
     * mode and the peer is alive remove the hook for both SPs.
     */
    if (b_remove_from_both) 
    {
        /* Set the transport server to the other SP.
         */
        status = fbe_api_sim_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Remove the debug hook on the peer also
         */
        status = fbe_api_scheduler_del_debug_hook(debug_hook_p->object_id,
                                              debug_hook_p->monitor_state,
                                              debug_hook_p->monitor_substate,
                                              debug_hook_p->val1,
                                              debug_hook_p->val2,
                                              debug_hook_p->check_type,
                                              debug_hook_p->action);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set the transport server to the this SP.
         */
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* If this is not a reboot then clear the debug hook.  If this is a reboot
     * we leave the hook for the passive (soon to be active) SP.
     */
    if ((b_is_reboot_test == FBE_FALSE)  ||
        (b_remove_from_both == FBE_TRUE)    )
    {
        fbe_zero_memory(debug_hook_p, sizeof(*debug_hook_p));
        debug_hook_p->object_id = FBE_OBJECT_ID_INVALID;
    }

    return FBE_STATUS_OK;
}
/***********************************************************
 * end fbe_test_sep_background_pause_remove_debug_hook()
 ***********************************************************/

/*!***************************************************************************
 * fbe_test_sep_background_pause_check_debug_hook()
 *****************************************************************************
 * @brief
 *  This function waits for rebuild debug hook to hit.
 *
 * @param   debug_hook_p - pointer to debug hook to check for
 * @param   b_is_debug_hook_reached_p - Pointer to indicate if
 *              complete or not. 
 *
 * @return  status
 *
 * @author
 *  03/26/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_pause_check_debug_hook(fbe_scheduler_debug_hook_t *debug_hook_p,
                                                            fbe_bool_t *b_is_debug_hook_reached_p)

{
    fbe_status_t                status = FBE_STATUS_OK;

    /* By defualt we haven't reached the debug hook
     */
    *b_is_debug_hook_reached_p = FBE_FALSE;

    /* Check if the debug hook has been reached or not.
     */
    status = fbe_api_scheduler_get_debug_hook(debug_hook_p);
    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    if (debug_hook_p->counter > 0)
    {
        *b_is_debug_hook_reached_p = FBE_TRUE;

        /* Display some information.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
               "fbe_test_sep_background_pause: Hit debug hook for obj: 0x%x monitor state: %d substate: %d counter: %d", 
               debug_hook_p->object_id, debug_hook_p->monitor_state, debug_hook_p->monitor_substate, (int)debug_hook_p->counter);
    }

    return FBE_STATUS_OK;
}
/*************************************************************
 * end fbe_test_sep_background_pause_check_debug_hook()
 *************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_set_debug_hook()
 *****************************************************************************
 *
 * @brief   First save the debug hook information in the structure provided
 *          then set the scheduler debug hook.
 *
 * @param   debug_hook_p - Pointer to debug hook information to populate 
 * @param   hook_index - For debug the index into the hook array
 * @param   object_id - The object id for the hook
 * @param   monitor_state - The monitor state for the hook
 * @param   monitor_substate - The monitor substate for the hook
 * @param   val1 - The first value to check for the hook
 * @param   val2 - The second vaue to check for the hook
 * @param   check_type - Check type for hook
 * @param   action - The action to take (pause, log etc) for the hook
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test
 *                             FBE_FALSE - This is not a reboot test
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/22/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_pause_set_debug_hook(fbe_scheduler_debug_hook_t *debug_hook_p,
                                                    fbe_u32_t hook_index,
                                                    fbe_object_id_t object_id,
                                                    fbe_u32_t monitor_state,
                                                    fbe_u32_t monitor_substate,
                                                    fbe_u64_t val1,
                                                    fbe_u64_t val2,
                                                    fbe_u32_t check_type,
                                                    fbe_u32_t action,
                                                    fbe_bool_t b_is_reboot_test)
{  
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_bool_t                              b_is_dualsp_mode = FBE_FALSE;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_bool_t                              b_add_to_both = FBE_FALSE;     
    fbe_cmi_service_get_info_t              cmi_info;
  
    /* Validate the hook index.
     */
    if (hook_index >= MAX_SCHEDULER_DEBUG_HOOKS)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Attempt to use index: %d greater than max index: %d",
                   __FUNCTION__, hook_index, (MAX_SCHEDULER_DEBUG_HOOKS -1));
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    status = fbe_api_cmi_service_get_info(&cmi_info);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    if ((b_is_dualsp_mode == FBE_TRUE)    &&
        (b_is_reboot_test == FBE_TRUE)    &&
        (cmi_info.peer_alive == FBE_TRUE)    )
    {
        b_add_to_both = FBE_TRUE;
    }

    /* Check if we need to clean the hook specified.
     */
    if ((debug_hook_p->object_id != 0)                     &&
        (debug_hook_p->object_id != FBE_OBJECT_ID_INVALID)    )
    {
        /* We don't expect the hook to be left set.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
               "background set debug hook: unexpected hook set index: %d obj: 0x%x state: %d substate: %d val1: 0x%llx val2: 0x%llx check type: %d action: %d",
               hook_index, debug_hook_p->object_id, debug_hook_p->monitor_state, debug_hook_p->monitor_substate, 
               (unsigned long long)debug_hook_p->val1, (unsigned long long)debug_hook_p->val2, debug_hook_p->check_type, debug_hook_p->action);

        /* Delete the hook to allow the copy to proceed. */
        status = fbe_api_scheduler_del_debug_hook(debug_hook_p->object_id,
                                                  debug_hook_p->monitor_state,
                                                  debug_hook_p->monitor_substate,
                                                  debug_hook_p->val1, debug_hook_p->val2, 
                                                  debug_hook_p->check_type, debug_hook_p->action);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = FBE_STATUS_GENERIC_FAILURE;
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Display some information
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "background set debug hook: debug SP: %d(%d) index: %d obj: 0x%x state: %d substate: %d val1: 0x%llx action: %d",
               this_sp, (b_add_to_both) ? other_sp : this_sp,
               hook_index, object_id, monitor_state, monitor_substate, (unsigned long long)val1, action);

    /* Populate the debug hook.
     */
    debug_hook_p->object_id = object_id;
    debug_hook_p->monitor_state = monitor_state;
    debug_hook_p->monitor_substate = monitor_substate;
    debug_hook_p->val1 = val1;
    debug_hook_p->val2 = val2;
    debug_hook_p->check_type = check_type;
    debug_hook_p->action = action;

    /* Now set/enable the debug hook.
     */
    status = fbe_api_scheduler_add_debug_hook(object_id,
                                              monitor_state,
                                              monitor_substate,
                                              val1,
                                              val2,              
                                              check_type,
                                              action);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Determine if we are in dualsp mode or not.  If we are in dualsp
     * mode and the peer is alive add the hook for both SPs.
     */
    if (b_add_to_both) 
    {
        /* Set the transport server to the other SP.
         */
        status = fbe_api_sim_transport_set_target_server(other_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Add the debug hook on the peer also
         */
        status = fbe_api_scheduler_add_debug_hook(object_id,
                                                  monitor_state,
                                                  monitor_substate,
                                                  val1,
                                                  val2,              
                                                  check_type,
                                                  action);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set the transport server to the this SP.
         */
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    return status;
}
/******************************************************************
 * end fbe_test_sep_background_set_debug_hook()
 ******************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_wait_for_and_remove_debug_hook()
 *****************************************************************************
 *
 * @brief   Wait for the debug hooks.  If not b_is_reboot_test we will remove
 *          the debug hooks.
 *
 * @param   rg_config_p - Pointer to array of raid group configs
 * @param   raid_group_count - Number of raid groups in array
 * @param   position_to_copy - The position for the copy request
 * @param   sep_params_p - Pointer to debug hook information
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test
 * @param   b_wait_for_object_ready - FBE_TRUE - Wait for object ready before
 *              we return.
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/22/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_wait_for_and_remove_debug_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                      fbe_u32_t raid_group_count,
                                                                      fbe_u32_t position_to_copy,
                                                                      fbe_sep_package_load_params_t *sep_params_p,
                                                                      fbe_bool_t b_is_reboot_test,
                                                                      fbe_bool_t b_wait_for_object_ready)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               rg_index;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_bool_t                              b_is_dualsp_mode = FBE_FALSE;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   debug_hook_sp;
    fbe_object_id_t                         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                               retry_count;
    fbe_u32_t                               max_retries = FBE_TEST_SEP_BACKGROUND_PAUSE_MAX_SHORT_RETRIES;
    fbe_bool_t                              raid_groups_complete[FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT];
    fbe_u32_t                               num_raid_groups_complete = 0;
    fbe_test_raid_group_disk_set_t         *drive_to_spare_p = NULL; 
    fbe_bool_t                              b_is_debug_hook_reached = FBE_FALSE;
    
    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    debug_hook_sp = this_sp;

    /* Determine if we are in dualsp mode or not.  If we are in dualsp
     * mode set the debug hook on `this' SP only since the other SP
     * maybe in the reset state.
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        debug_hook_sp = this_sp;
    }

    /* Validate the debug hooks.
     */
    if ((sep_params_p == NULL)                                                ||
        (sep_params_p->scheduler_hooks[0].object_id == 0)                     ||
        (sep_params_p->scheduler_hooks[0].object_id == FBE_OBJECT_ID_INVALID)    )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Debug hook: %p unexpected ", 
                   __FUNCTION__, sep_params_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Set the transport server to the correct SP.
     */
    status = fbe_api_sim_transport_set_target_server(debug_hook_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize complete array.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        raid_groups_complete[rg_index] = FBE_FALSE;
    }

    /*  Loop until we either see all the proactive copies start or timeout
     */
    for (retry_count = 0; (retry_count < max_retries) && (num_raid_groups_complete < raid_group_count); retry_count++)
    {
        /* For all raid groups simply check */
        current_rg_config_p = rg_config_p;
        for (rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            /* Skip raid groups that have completed the event.
             */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }

            /* Skip raid groups that are not enabled.
             */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                raid_groups_complete[rg_index] = FBE_TRUE;
                num_raid_groups_complete++;
                current_rg_config_p++;
                continue;
            }

            /* Validate sparing position.
             */
            if ((position_to_copy == FBE_TEST_SEP_BACKGROUND_PAUSE_INVALID_DISK_POSITION) ||
                (position_to_copy >= current_rg_config_p->width)                             )
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid copy position: %d",
                           __FUNCTION__, position_to_copy);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                status = fbe_api_sim_transport_set_target_server(this_sp);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return FBE_STATUS_GENERIC_FAILURE; 
            }

            /* Get the vd object id of the position to force into proactive copy
             */
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

            /* Check if we have reached the debug hook.
             */
            status = fbe_test_sep_background_pause_check_debug_hook(&sep_params_p->scheduler_hooks[rg_index],
                                                                    &b_is_debug_hook_reached);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* If we have reached the debug hook log a message.
             */
            if (b_is_debug_hook_reached == FBE_TRUE)
            {
                /* Flag the debug hook reached.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "fbe_test_sep_background_pause: Hit debug hook raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                           rg_object_id, position_to_copy, vd_object_id,
                           drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);
                raid_groups_complete[rg_index] = FBE_TRUE;
                num_raid_groups_complete++;

                /* Immediately remove the debug hook.
                 */
                status = fbe_test_sep_background_pause_remove_debug_hook(&sep_params_p->scheduler_hooks[rg_index], rg_index,
                                                                         b_is_reboot_test);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            }

            /* Goto next raid group
             */                
            current_rg_config_p++;

        } /* end for all raid groups */

        /* If not done sleep for a short period
         */
        if (num_raid_groups_complete < raid_group_count)
        {
            fbe_api_sleep(FBE_TEST_SEP_BACKGROUND_PAUSE_SHORT_RETRY_TIME);
        }

    } /* end while not complete or retries exhausted */

    /* Check if we are not complete.
     */
    if ((retry_count >= max_retries)                  || 
        (num_raid_groups_complete < raid_group_count)    )
    {
        /* Go thru all the raid groups and report the ones that timed out.
         */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            /* Skip raid groups that are not enabled.
             */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                current_rg_config_p++;
                continue;
            }

            /* Skip raid groups that have completed the event.
             */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }

            /* Using the position supplied, get the virtual drive information.
             */
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

            /* Display some information
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "fbe_test_sep_background_pause: Timed out debug hook state: %d substate: %d raid: 0x%x pos: %d vd: 0x%x (%d_%d_%d)",
                       sep_params_p->scheduler_hooks[rg_index].monitor_state, sep_params_p->scheduler_hooks[rg_index].monitor_substate,
                       rg_object_id, position_to_copy, vd_object_id,
                       drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);

            /* Goto the next raid group
             */
            current_rg_config_p++;

        } /* For all raid groups */

        /* Print a message then fail.
         */
        status = FBE_STATUS_TIMEOUT;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: %d raid groups out of: %d took longer than: %d seconds to hit debug hook",
                   (raid_group_count - num_raid_groups_complete), raid_group_count, 
                   ((FBE_TEST_SEP_BACKGROUND_PAUSE_MAX_SHORT_RETRIES * FBE_TEST_SEP_BACKGROUND_PAUSE_SHORT_RETRY_TIME) / 1000));

        /* Generate an error trace on the SP and fail the test.
         */
        fbe_test_sep_util_raid_group_generate_error_trace(rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set the SP back to the original.
         */
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If enabled wait for the associated virtual drive to be READY before 
     * we proceed.
     */
    if (b_wait_for_object_ready == FBE_TRUE)
    {

        current_rg_config_p = rg_config_p;
        for (rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            fbe_u32_t   removed_index;
            fbe_bool_t  b_object_removed;

            b_object_removed = FBE_FALSE;
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                current_rg_config_p++;
                continue;
            }
        
            /* Using the position supplied, get the virtual drive information.
             */
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);
    
            /*! @note Some of the copy tests remove the copy source drive which
             *        results in the virtual drive going to the failed state.
             *        If the virtual drive is removed validate that it is in
             *        the failed state.
             */
            for (removed_index = 0; removed_index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; removed_index++)
            {
                if (current_rg_config_p->drives_removed_array[removed_index] == position_to_copy)
                {
                    b_object_removed = FBE_TRUE;
                    break;
                }
            }

            /* Wait for the virtual drive to be ready before continuing.
             */
            if (b_object_removed == FBE_FALSE)
            {
                status = fbe_test_common_util_wait_for_object_lifecycle_state_on_sp(vd_object_id, 
                                                                                    debug_hook_sp,
                                                                                    FBE_LIFECYCLE_STATE_READY, 
                                                                                    20000, 
                                                                                    FBE_PACKAGE_ID_SEP_0);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
    
            /* Goto next raid group.
             */
            current_rg_config_p++;
        }
    }

    /* Set the transport server to `this SP'.
     */
    status = fbe_api_sim_transport_set_target_server(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/******************************************************************
 * end fbe_test_sep_background_pause_wait_for_and_remove_debug_hook()
 ******************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_set_private_copy_complete_hook()
 *****************************************************************************
 *
 * @brief   To guarantee that we don't over-run any state (by completing the
 *          copy), we set a debug hook to stop before we swap out the source
 *          drive.                                 
 *
 * @param   rg_config_p - Current raid group config
 * @param   raid_group_count - The index of the raid group arrays to set debug hook for
 * @param   position_to_copy - Raid group position that is being spared
 * @param   b_is_reboot_test - FBE_TRUE this is a reboot test
 *                             FBE_FALSE this is not a reboot test
 *
 * @return  fbe_status_t 
 *
 * @author
 *  06/26/2012  - Ron Proulx    - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_set_private_copy_complete_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                   fbe_u32_t raid_group_count,
                                                                   fbe_u32_t position_to_copy,
                                                                   fbe_sep_package_load_params_t *sep_params_p,
                                                                   fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       rebuild_hook_index = 0;
    fbe_u32_t                       rg_index;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                      b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode(); 

    /*! @note For dualsp and reboot don't set any hooks.
     */
    if ((b_is_dualsp_mode == FBE_TRUE) &&
        (b_is_reboot_test == FBE_TRUE)    )
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Both dualsp: %d and reboot: %d set - skip", 
                   __FUNCTION__, b_is_dualsp_mode, b_is_reboot_test);
        return FBE_STATUS_OK;
    }

    /* If the rebuild hook is already set something is wrong.
     */
    if (fbe_test_sep_background_pause_b_is_private_rebuild_hook_set == FBE_TRUE)
    {
        /* We don't expected the rebuild hook to already be set.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Rebuild hook already set!", __FUNCTION__);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Set the rebuild hook for each position
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Set the rebuild hook.
         */
        rebuild_hook_index = raid_group_count + rg_index;
        MUT_ASSERT_TRUE(rebuild_hook_index < MAX_SCHEDULER_DEBUG_HOOKS);
        status = fbe_test_sep_background_pause_set_debug_hook(&sep_params_p->scheduler_hooks[rebuild_hook_index], rebuild_hook_index,
                                                        vd_object_id,
                                                        SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                                        FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE,
                                                        0, 0, 
                                                        SCHEDULER_CHECK_STATES,
                                                        SCHEDULER_DEBUG_ACTION_PAUSE,
                                                        b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Flag the setting the private rebuild hook
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Set private rebuild hook raid obj: 0x%x pos: %d vd obj: 0x%x",
                   rg_object_id, position_to_copy, vd_object_id);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Flag the fact that the rebuild hook has already been set.
     */
    fbe_test_sep_background_pause_b_is_private_rebuild_hook_set = FBE_TRUE;

    /* Return the status
     */
    return status;
}
/********************************************************************
 * end fbe_test_sep_background_pause_set_private_copy_complete_hook()
 ********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_set_initiate_copy_complete_hook()
 *****************************************************************************
 *
 * @brief   User requested pause at `initiate copy complete(or abort)' job start                       
 *
 * @param   rg_config_p - Current raid group config
 * @param   raid_group_count - The index of the raid group arrays to set debug hook for
 * @param   position_to_copy - Raid group position that is being spared
 * @param   b_is_reboot_test - FBE_TRUE this is a reboot test
 *                             FBE_FALSE this is not a reboot test
 *
 * @return  fbe_status_t 
 *
 * @author
 *  11/14/2012  - Ron Proulx    - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_set_initiate_copy_complete_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                    fbe_u32_t raid_group_count,
                                                                    fbe_u32_t position_to_copy,
                                                                    fbe_sep_package_load_params_t *sep_params_p,
                                                                    fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       rg_index;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                      b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /*! @note For dualsp and reboot don't set any hooks.
     */
    if ((b_is_dualsp_mode == FBE_TRUE) &&
        (b_is_reboot_test == FBE_TRUE)    )
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Both dualsp: %d and reboot: %d set - skip", 
                   __FUNCTION__, b_is_dualsp_mode, b_is_reboot_test);
        return FBE_STATUS_OK;
    }

    /* If the swap-out hook is already set something is wrong.
     */
    if (fbe_test_sep_background_pause_b_is_initiate_copy_complete_hook_set == FBE_TRUE)
    {
        /* We don't expected the rebuild hook to already be set.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Rebuild hook already set!", __FUNCTION__);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Set the rebuild hook for each position
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Set the swap-out
         */
        MUT_ASSERT_TRUE(rg_index < MAX_SCHEDULER_DEBUG_HOOKS);
        status = fbe_test_sep_background_pause_set_debug_hook(&sep_params_p->scheduler_hooks[rg_index], rg_index,
                                                        vd_object_id,
                                                        SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                                        FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE,
                                                        0, 0, 
                                                        SCHEDULER_CHECK_STATES,
                                                        SCHEDULER_DEBUG_ACTION_PAUSE,
                                                        b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Flag the setting the private rebuild hook
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Set initiate copy complete hook raid obj: 0x%x pos: %d vd obj: 0x%x",
                   rg_object_id, position_to_copy, vd_object_id);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Flag the fact that the initiate copy complete hook has already been set.
     */
    fbe_test_sep_background_pause_b_is_initiate_copy_complete_hook_set = FBE_TRUE;

    /* Return the status
     */
    return status;
}
/********************************************************************
 * end fbe_test_sep_background_pause_set_initiate_copy_complete_hook()
 ********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_set_swap_out_complete_hook()
 *****************************************************************************
 *
 * @brief   User requested pause after the source drive has been swapped out                   
 *
 * @param   rg_config_p - Current raid group config
 * @param   raid_group_count - The index of the raid group arrays to set debug hook for
 * @param   position_to_copy - Raid group position that is being spared
 * @param   b_is_reboot_test - FBE_TRUE this is a reboot test
 *                             FBE_FALSE this is not a reboot test
 *
 * @return  fbe_status_t 
 *
 * @author
 *  06/02/2014  - Ron Proulx    - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_set_swap_out_complete_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                    fbe_u32_t raid_group_count,
                                                                    fbe_u32_t position_to_copy,
                                                                    fbe_sep_package_load_params_t *sep_params_p,
                                                                    fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       rg_index;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;

    /* If the swap-out hook is already set something is wrong.
     */
    if (fbe_test_sep_background_pause_b_is_swap_out_complete_hook_set == FBE_TRUE)
    {
        /* We don't expected the rebuild hook to already be set.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Rebuild hook already set!", __FUNCTION__);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Set the rebuild hook for each position
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Set the swap-out
         */
        MUT_ASSERT_TRUE(rg_index < MAX_SCHEDULER_DEBUG_HOOKS);
        status = fbe_test_sep_background_pause_set_debug_hook(&sep_params_p->scheduler_hooks[rg_index], rg_index,
                                                              vd_object_id,
                                                              SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT, 
                                                              FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE,
                                                              0, 0, 
                                                              SCHEDULER_CHECK_STATES,
                                                              SCHEDULER_DEBUG_ACTION_PAUSE,
                                                              b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Flag the setting the private rebuild hook
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Set swap-out complete hook raid obj: 0x%x pos: %d vd obj: 0x%x",
                   rg_object_id, position_to_copy, vd_object_id);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Flag the fact that the swap-out complete hook has already been set.
     */
    fbe_test_sep_background_pause_b_is_swap_out_complete_hook_set = FBE_TRUE;

    /* Return the status
     */
    return status;
}
/********************************************************************
 * end fbe_test_sep_background_pause_set_swap_out_complete_hook()
 ********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_wait_and_clear_private_copy_complete_hook()
 *****************************************************************************
 *
 * @brief   Wait for the rebuild hooks.  If not b_is_reboot_test we will remove
 *          the debug hooks.
 *
 * @param   rg_config_p - Pointer to array of raid group configs
 * @param   raid_group_count - Number of raid groups in array
 * @param   position_to_copy - The position for the copy request
 * @param   sep_params_p - Pointer to debug hook information
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/22/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_wait_and_clear_private_copy_complete_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                      fbe_u32_t raid_group_count,
                                                                      fbe_u32_t position_to_copy,
                                                                      fbe_sep_package_load_params_t *sep_params_p,
                                                                      fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               rg_index;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   debug_hook_sp;
    fbe_object_id_t                         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                               retry_count;
    fbe_u32_t                               max_retries = FBE_TEST_SEP_BACKGROUND_PAUSE_MAX_SHORT_RETRIES;
    fbe_bool_t                              raid_groups_complete[FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT];
    fbe_u32_t                               num_raid_groups_complete = 0;
    fbe_test_raid_group_disk_set_t         *drive_to_spare_p = NULL; 
    fbe_bool_t                              b_is_debug_hook_reached = FBE_FALSE;
    fbe_u32_t                               rebuild_hook_index = 0;
    fbe_bool_t                              b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode(); 

    /*! @note For dualsp and reboot don't set any hooks.
     */
    if ((b_is_dualsp_mode == FBE_TRUE) &&
        (b_is_reboot_test == FBE_TRUE)    )
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Both dualsp: %d and reboot: %d set - skip", 
                   __FUNCTION__, b_is_dualsp_mode, b_is_reboot_test);
        return FBE_STATUS_OK;
    }

    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    debug_hook_sp = this_sp;

    /* Determine if we are in dualsp mode or not.  If we are in dualsp
     * mode set the debug hook on `this' SP only since the other SP
     * maybe in the reset state.
     */
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        debug_hook_sp = this_sp;
    }

    /* Validate the rebuild hook which are located after any debug hooks.
     */
    rebuild_hook_index = raid_group_count;
    if ((sep_params_p == NULL)                                                                 ||
        (sep_params_p->scheduler_hooks[rebuild_hook_index].object_id == 0)                     ||
        (sep_params_p->scheduler_hooks[rebuild_hook_index].object_id == FBE_OBJECT_ID_INVALID) ||
        (fbe_test_sep_background_pause_b_is_private_rebuild_hook_set != FBE_TRUE)                         )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Rebuild hook: %p set: %d unexpected ", 
                   __FUNCTION__, sep_params_p, fbe_test_sep_background_pause_b_is_private_rebuild_hook_set);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Set the transport server to the correct SP.
     */
    status = fbe_api_sim_transport_set_target_server(debug_hook_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize complete array.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        raid_groups_complete[rg_index] = FBE_FALSE;
    }

    /*  Loop until we either see all the proactive copies start or timeout
     */
    for (retry_count = 0; (retry_count < max_retries) && (num_raid_groups_complete < raid_group_count); retry_count++)
    {
        /* For all raid groups simply check */
        current_rg_config_p = rg_config_p;
        for (rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            /* Skip raid groups that have completed the event.
             */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }

            /* Skip raid groups that are not enabled.
             */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                raid_groups_complete[rg_index] = FBE_TRUE;
                num_raid_groups_complete++;
                current_rg_config_p++;
                continue;
            }

            /* Validate sparing position.
             */
            if ((position_to_copy == FBE_TEST_SEP_BACKGROUND_PAUSE_INVALID_DISK_POSITION) ||
                (position_to_copy >= current_rg_config_p->width)                             )
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid copy position: %d",
                           __FUNCTION__, position_to_copy);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                status = fbe_api_sim_transport_set_target_server(this_sp);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return FBE_STATUS_GENERIC_FAILURE; 
            }

            /* Get the vd object id of the position to force into proactive copy
             */
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

            /* Check if we have reached the debug hook.
             */
            rebuild_hook_index = raid_group_count + rg_index;
            status = fbe_test_sep_background_pause_check_debug_hook(&sep_params_p->scheduler_hooks[rebuild_hook_index],
                                                                    &b_is_debug_hook_reached);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* If we have reached the debug hook log a message.
             */
            if (b_is_debug_hook_reached == FBE_TRUE)
            {
                /* Flag the debug hook reached.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "fbe_test_sep_background_pause: Hit private rebuild hook raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                           rg_object_id, position_to_copy, vd_object_id,
                           drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);
                raid_groups_complete[rg_index] = FBE_TRUE;
                num_raid_groups_complete++;

                /* Immediately remove the debug hook.
                 */
                status = fbe_test_sep_background_pause_remove_debug_hook(&sep_params_p->scheduler_hooks[rebuild_hook_index], rebuild_hook_index,
                                                                         b_is_reboot_test);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            }

            /* Goto next raid group
             */                
            current_rg_config_p++;

        } /* end for all raid groups */

        /* If not done sleep for a short period
         */
        if (num_raid_groups_complete < raid_group_count)
        {
            fbe_api_sleep(FBE_TEST_SEP_BACKGROUND_PAUSE_SHORT_RETRY_TIME);
        }

    } /* end while not complete or retries exhausted */

    /* Check if we are not complete.
     */
    if ((retry_count >= max_retries)                  || 
        (num_raid_groups_complete < raid_group_count)    )
    {
        /* Go thru all the raid groups and report the ones that timed out.
         */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            /* Skip raid groups that are not enabled.
             */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                current_rg_config_p++;
                continue;
            }

            /* Skip raid groups that have completed the event.
             */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }

            /* Using the position supplied, get the virtual drive information.
             */
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

            /* Display some information
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "fbe_test_sep_background_pause: Timed out private rebuild hook: raid: 0x%x pos: %d vd: 0x%x (%d_%d_%d)",
                       rg_object_id, position_to_copy, vd_object_id,
                       drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);

            /* Goto the next raid group
             */
            current_rg_config_p++;

        } /* For all raid groups */

        /* Print a message then fail.
         */
        status = FBE_STATUS_TIMEOUT;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: %d raid groups out of: %d took longer than: %d seconds to hit private rebuild hook",
                   (raid_group_count - num_raid_groups_complete), raid_group_count, 
                   ((FBE_TEST_SEP_BACKGROUND_PAUSE_MAX_SHORT_RETRIES * FBE_TEST_SEP_BACKGROUND_PAUSE_SHORT_RETRY_TIME) / 1000));

        /* Generate an error trace on the SP and fail the test.
         */
        fbe_test_sep_util_raid_group_generate_error_trace(rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set the SP back to the original.
         */
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Wait for the associated virtual drive to be READY before we proceed.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
    
        /* Using the position supplied, get the virtual drive information.
         */
        drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Wait for the virtual drive to be ready before continuing.
         */
        status = fbe_test_common_util_wait_for_object_lifecycle_state_on_sp(vd_object_id, 
                                                                            debug_hook_sp,
                                                                            FBE_LIFECYCLE_STATE_READY, 
                                                                            20000, 
                                                                            FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Set the transport server to `this SP'.
     */
    status = fbe_api_sim_transport_set_target_server(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Clear the rebuild hook.
     */
    fbe_test_sep_background_pause_b_is_private_rebuild_hook_set = FBE_FALSE;

    return status;
}
/*******************************************************************************
 * end fbe_test_sep_background_pause_wait_and_clear_private_copy_complete_hook()
 *******************************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_wait_and_clear_initiate_copy_complete_hook()
 *****************************************************************************
 *
 * @brief   Wait for the swap-out hooks.  If not b_is_reboot_test we will remove
 *          the debug hooks.
 *
 * @param   rg_config_p - Pointer to array of raid group configs
 * @param   raid_group_count - Number of raid groups in array
 * @param   position_to_copy - The position for the copy request
 * @param   sep_params_p - Pointer to debug hook information
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test
 *
 * @return fbe_status_t 
 *
 * @author
 *  11/14/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_wait_and_clear_initiate_copy_complete_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                      fbe_u32_t raid_group_count,
                                                                      fbe_u32_t position_to_copy,
                                                                      fbe_sep_package_load_params_t *sep_params_p,
                                                                      fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               rg_index = 0;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   debug_hook_sp;
    fbe_object_id_t                         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                               retry_count;
    fbe_u32_t                               max_retries = FBE_TEST_SEP_BACKGROUND_PAUSE_MAX_SHORT_RETRIES;
    fbe_bool_t                              raid_groups_complete[FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT];
    fbe_u32_t                               num_raid_groups_complete = 0;
    fbe_test_raid_group_disk_set_t         *drive_to_spare_p = NULL; 
    fbe_bool_t                              b_is_debug_hook_reached = FBE_FALSE;
    fbe_bool_t                              b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /*! @note For dualsp and reboot don't set any hooks.
     */
    if ((b_is_dualsp_mode == FBE_TRUE) &&
        (b_is_reboot_test == FBE_TRUE)    )
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Both dualsp: %d and reboot: %d set - skip", 
                   __FUNCTION__, b_is_dualsp_mode, b_is_reboot_test);
        return FBE_STATUS_OK;
    }
       
    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    debug_hook_sp = this_sp;

    /* Determine if we are in dualsp mode or not.  If we are in dualsp
     * mode set the debug hook on `this' SP only since the other SP
     * maybe in the reset state.
     */
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        debug_hook_sp = this_sp;
    }

    /* Validate the swap-out hook is first
     */
    if ((sep_params_p == NULL)                                                       ||
        (sep_params_p->scheduler_hooks[rg_index].object_id == 0)                     ||
        (sep_params_p->scheduler_hooks[rg_index].object_id == FBE_OBJECT_ID_INVALID) ||
        (fbe_test_sep_background_pause_b_is_initiate_copy_complete_hook_set != FBE_TRUE)              )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Swap-out hook: %p set: %d unexpected ", 
                   __FUNCTION__, sep_params_p, fbe_test_sep_background_pause_b_is_initiate_copy_complete_hook_set);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Set the transport server to the correct SP.
     */
    status = fbe_api_sim_transport_set_target_server(debug_hook_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize complete array.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        raid_groups_complete[rg_index] = FBE_FALSE;
    }

    /*  Loop until we either see all the proactive copies start or timeout
     */
    for (retry_count = 0; (retry_count < max_retries) && (num_raid_groups_complete < raid_group_count); retry_count++)
    {
        /* For all raid groups simply check */
        current_rg_config_p = rg_config_p;
        for (rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            /* Skip raid groups that have completed the event.
             */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }

            /* Skip raid groups that are not enabled.
             */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                raid_groups_complete[rg_index] = FBE_TRUE;
                num_raid_groups_complete++;
                current_rg_config_p++;
                continue;
            }

            /* Validate sparing position.
             */
            if ((position_to_copy == FBE_TEST_SEP_BACKGROUND_PAUSE_INVALID_DISK_POSITION) ||
                (position_to_copy >= current_rg_config_p->width)                             )
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid copy position: %d",
                           __FUNCTION__, position_to_copy);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                status = fbe_api_sim_transport_set_target_server(this_sp);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return FBE_STATUS_GENERIC_FAILURE; 
            }

            /* Get the vd object id of the position to force into proactive copy
             */
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

            /* Check if we have reached the debug hook.
             */
            status = fbe_test_sep_background_pause_check_debug_hook(&sep_params_p->scheduler_hooks[rg_index],
                                                                    &b_is_debug_hook_reached);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* If we have reached the debug hook log a message.
             */
            if (b_is_debug_hook_reached == FBE_TRUE)
            {
                /* Flag the debug hook reached.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "fbe_test_sep_background_pause: Hit swap-out hook raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                           rg_object_id, position_to_copy, vd_object_id,
                           drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);
                raid_groups_complete[rg_index] = FBE_TRUE;
                num_raid_groups_complete++;

                /* Immediately remove the debug hook.
                 */
                status = fbe_test_sep_background_pause_remove_debug_hook(&sep_params_p->scheduler_hooks[rg_index], rg_index,
                                                                         b_is_reboot_test);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            }

            /* Goto next raid group
             */                
            current_rg_config_p++;

        } /* end for all raid groups */

        /* If not done sleep for a short period
         */
        if (num_raid_groups_complete < raid_group_count)
        {
            fbe_api_sleep(FBE_TEST_SEP_BACKGROUND_PAUSE_SHORT_RETRY_TIME);
        }

    } /* end while not complete or retries exhausted */

    /* Check if we are not complete.
     */
    if ((retry_count >= max_retries)                  || 
        (num_raid_groups_complete < raid_group_count)    )
    {
        /* Go thru all the raid groups and report the ones that timed out.
         */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            /* Skip raid groups that are not enabled.
             */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                current_rg_config_p++;
                continue;
            }

            /* Skip raid groups that have completed the event.
             */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }

            /* Using the position supplied, get the virtual drive information.
             */
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

            /* Display some information
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "fbe_test_sep_background_pause: Timed out swap-out hook: raid: 0x%x pos: %d vd: 0x%x (%d_%d_%d)",
                       rg_object_id, position_to_copy, vd_object_id,
                       drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);

            /* Goto the next raid group
             */
            current_rg_config_p++;

        } /* For all raid groups */

        /* Print a message then fail.
         */
        status = FBE_STATUS_TIMEOUT;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: %d raid groups out of: %d took longer than: %d seconds to hit swap-out hook",
                   (raid_group_count - num_raid_groups_complete), raid_group_count, 
                   ((FBE_TEST_SEP_BACKGROUND_PAUSE_MAX_SHORT_RETRIES * FBE_TEST_SEP_BACKGROUND_PAUSE_SHORT_RETRY_TIME) / 1000));

        /* Generate an error trace on the SP and fail the test.
         */
        fbe_test_sep_util_raid_group_generate_error_trace(rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set the SP back to the original.
         */
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Wait for the associated virtual drive to be READY before we proceed.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
    
        /* Using the position supplied, get the virtual drive information.
         */
        drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Wait for the virtual drive to be ready before continuing.
         */
        status = fbe_test_common_util_wait_for_object_lifecycle_state_on_sp(vd_object_id, 
                                                                            debug_hook_sp,
                                                                            FBE_LIFECYCLE_STATE_READY, 
                                                                            20000, 
                                                                            FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Set the transport server to `this SP'.
     */
    status = fbe_api_sim_transport_set_target_server(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Clear the swap-out hook.
     */
    fbe_test_sep_background_pause_b_is_initiate_copy_complete_hook_set = FBE_FALSE;

    return status;
}
/*******************************************************************************
 * end fbe_test_sep_background_pause_wait_and_clear_initiate_copy_complete_hook()
 *******************************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_wait_and_clear_swap_out_complete_hook()
 *****************************************************************************
 *
 * @brief   Wait for the swap-out hooks.  If not b_is_reboot_test we will remove
 *          the debug hooks.
 *
 * @param   rg_config_p - Pointer to array of raid group configs
 * @param   raid_group_count - Number of raid groups in array
 * @param   position_to_copy - The position for the copy request
 * @param   sep_params_p - Pointer to debug hook information
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test
 *
 * @return fbe_status_t 
 *
 * @author
 *  11/14/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_wait_and_clear_swap_out_complete_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                      fbe_u32_t raid_group_count,
                                                                      fbe_u32_t position_to_copy,
                                                                      fbe_sep_package_load_params_t *sep_params_p,
                                                                      fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               rg_index = 0;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   debug_hook_sp;
    fbe_object_id_t                         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                               retry_count;
    fbe_u32_t                               max_retries = FBE_TEST_SEP_BACKGROUND_PAUSE_MAX_SHORT_RETRIES;
    fbe_bool_t                              raid_groups_complete[FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT];
    fbe_u32_t                               num_raid_groups_complete = 0;
    fbe_test_raid_group_disk_set_t         *drive_to_spare_p = NULL; 
    fbe_bool_t                              b_is_debug_hook_reached = FBE_FALSE;
    fbe_bool_t                              b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /*! @note For dualsp and reboot don't set any hooks.
     */
    if ((b_is_dualsp_mode == FBE_TRUE) &&
        (b_is_reboot_test == FBE_TRUE)    )
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Both dualsp: %d and reboot: %d set - skip", 
                   __FUNCTION__, b_is_dualsp_mode, b_is_reboot_test);
        return FBE_STATUS_OK;
    }
       
    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    debug_hook_sp = this_sp;

    /* Determine if we are in dualsp mode or not.  If we are in dualsp
     * mode set the debug hook on `this' SP only since the other SP
     * maybe in the reset state.
     */
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        debug_hook_sp = this_sp;
    }

    /* Validate the swap-out hook is first
     */
    if ((sep_params_p == NULL)                                                       ||
        (sep_params_p->scheduler_hooks[rg_index].object_id == 0)                     ||
        (sep_params_p->scheduler_hooks[rg_index].object_id == FBE_OBJECT_ID_INVALID) ||
        (fbe_test_sep_background_pause_b_is_swap_out_complete_hook_set != FBE_TRUE)              )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Swap-out hook: %p set: %d unexpected ", 
                   __FUNCTION__, sep_params_p, fbe_test_sep_background_pause_b_is_initiate_copy_complete_hook_set);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Set the transport server to the correct SP.
     */
    status = fbe_api_sim_transport_set_target_server(debug_hook_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize complete array.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        raid_groups_complete[rg_index] = FBE_FALSE;
    }

    /*  Loop until we either see all the proactive copies start or timeout
     */
    for (retry_count = 0; (retry_count < max_retries) && (num_raid_groups_complete < raid_group_count); retry_count++)
    {
        /* For all raid groups simply check */
        current_rg_config_p = rg_config_p;
        for (rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            /* Skip raid groups that have completed the event.
             */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }

            /* Skip raid groups that are not enabled.
             */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                raid_groups_complete[rg_index] = FBE_TRUE;
                num_raid_groups_complete++;
                current_rg_config_p++;
                continue;
            }

            /* Validate sparing position.
             */
            if ((position_to_copy == FBE_TEST_SEP_BACKGROUND_PAUSE_INVALID_DISK_POSITION) ||
                (position_to_copy >= current_rg_config_p->width)                             )
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid copy position: %d",
                           __FUNCTION__, position_to_copy);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                status = fbe_api_sim_transport_set_target_server(this_sp);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return FBE_STATUS_GENERIC_FAILURE; 
            }

            /* Get the vd object id of the position to force into proactive copy
             */
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

            /* Check if we have reached the debug hook.
             */
            status = fbe_test_sep_background_pause_check_debug_hook(&sep_params_p->scheduler_hooks[rg_index],
                                                                    &b_is_debug_hook_reached);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* If we have reached the debug hook log a message.
             */
            if (b_is_debug_hook_reached == FBE_TRUE)
            {
                /* Flag the debug hook reached.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "fbe_test_sep_background_pause: Hit swap-out hook raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                           rg_object_id, position_to_copy, vd_object_id,
                           drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);
                raid_groups_complete[rg_index] = FBE_TRUE;
                num_raid_groups_complete++;

                /* Immediately remove the debug hook.
                 */
                status = fbe_test_sep_background_pause_remove_debug_hook(&sep_params_p->scheduler_hooks[rg_index], rg_index,
                                                                         b_is_reboot_test);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            }

            /* Goto next raid group
             */                
            current_rg_config_p++;

        } /* end for all raid groups */

        /* If not done sleep for a short period
         */
        if (num_raid_groups_complete < raid_group_count)
        {
            fbe_api_sleep(FBE_TEST_SEP_BACKGROUND_PAUSE_SHORT_RETRY_TIME);
        }

    } /* end while not complete or retries exhausted */

    /* Check if we are not complete.
     */
    if ((retry_count >= max_retries)                  || 
        (num_raid_groups_complete < raid_group_count)    )
    {
        /* Go thru all the raid groups and report the ones that timed out.
         */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            /* Skip raid groups that are not enabled.
             */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                current_rg_config_p++;
                continue;
            }

            /* Skip raid groups that have completed the event.
             */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }

            /* Using the position supplied, get the virtual drive information.
             */
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

            /* Display some information
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "fbe_test_sep_background_pause: Timed out swap-out hook: raid: 0x%x pos: %d vd: 0x%x (%d_%d_%d)",
                       rg_object_id, position_to_copy, vd_object_id,
                       drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);

            /* Goto the next raid group
             */
            current_rg_config_p++;

        } /* For all raid groups */

        /* Print a message then fail.
         */
        status = FBE_STATUS_TIMEOUT;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: %d raid groups out of: %d took longer than: %d seconds to hit swap-out hook",
                   (raid_group_count - num_raid_groups_complete), raid_group_count, 
                   ((FBE_TEST_SEP_BACKGROUND_PAUSE_MAX_SHORT_RETRIES * FBE_TEST_SEP_BACKGROUND_PAUSE_SHORT_RETRY_TIME) / 1000));

        /* Generate an error trace on the SP and fail the test.
         */
        fbe_test_sep_util_raid_group_generate_error_trace(rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set the SP back to the original.
         */
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Wait for the associated virtual drive to be READY before we proceed.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
    
        /* Using the position supplied, get the virtual drive information.
         */
        drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Wait for the virtual drive to be ready before continuing.
         */
        status = fbe_test_common_util_wait_for_object_lifecycle_state_on_sp(vd_object_id, 
                                                                            debug_hook_sp,
                                                                            FBE_LIFECYCLE_STATE_READY, 
                                                                            20000, 
                                                                            FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Set the transport server to `this SP'.
     */
    status = fbe_api_sim_transport_set_target_server(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Clear the swap-out hook.
     */
    fbe_test_sep_background_pause_b_is_swap_out_complete_hook_set = FBE_FALSE;

    return status;
}
/*******************************************************************************
 * end fbe_test_sep_background_pause_wait_and_clear_swap_out_complete_hook()
 *******************************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_remove_private_copy_complete_hook()
 *****************************************************************************
 *
 * @brief   To guarantee that we don't over-run any state (by completing the
 *          copy), we set a debug hook to stop before we swap out the source
 *          drive.  Now we remove them (we do not wait for the hook).                                
 *
 * @param   rg_config_p - Current raid group config
 * @param   raid_group_count - The index of the raid group arrays to set debug hook for
 * @param   position_to_copy - Raid group position that is being spared
 * @param   b_is_reboot_test - FBE_TRUE this is a reboot test
 *                             FBE_FALSE this is not a reboot test
 *
 * @return  fbe_status_t 
 *
 * @author
 *  06/26/2012  - Ron Proulx    - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_remove_private_copy_complete_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                   fbe_u32_t raid_group_count,
                                                                   fbe_u32_t position_to_copy,
                                                                   fbe_sep_package_load_params_t *sep_params_p,
                                                                   fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       rebuild_hook_index = 0;
    fbe_u32_t                       rg_index;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                      b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode(); 

    /*! @note For dualsp and reboot don't set any hooks.
     */
    if ((b_is_dualsp_mode == FBE_TRUE) &&
        (b_is_reboot_test == FBE_TRUE)    )
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Both dualsp: %d and reboot: %d set - skip", 
                   __FUNCTION__, b_is_dualsp_mode, b_is_reboot_test);
        return FBE_STATUS_OK;
    }

    /* Only remove the rebuild hook if it is set.
     */
    if (fbe_test_sep_background_pause_b_is_private_rebuild_hook_set == FBE_FALSE)
    {
        /* Rebuild hook not set.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Rebuild hook not set.", __FUNCTION__);
        return FBE_STATUS_OK;
    }

    /* Set the rebuild hook for each position
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Remove the passed copy of the debug hook.
         */
        rebuild_hook_index = raid_group_count + rg_index;
        MUT_ASSERT_TRUE(rebuild_hook_index < MAX_SCHEDULER_DEBUG_HOOKS);
        status = fbe_test_sep_background_pause_remove_debug_hook(&sep_params_p->scheduler_hooks[rebuild_hook_index], rebuild_hook_index,
                                                                 b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Flag the setting the private rebuild hook
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Remove private rebuild hook raid obj: 0x%x pos: %d vd obj: 0x%x",
                   rg_object_id, position_to_copy, vd_object_id);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Clear the rebuild hook.
     */
    fbe_test_sep_background_pause_b_is_private_rebuild_hook_set = FBE_FALSE;

    /* Return the status
     */
    return status;
}
/***********************************************************************
 * end fbe_test_sep_background_pause_remove_private_copy_complete_hook()
 ***********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_remove_initiate_copy_complete_hook()
 *****************************************************************************
 *
 * @brief   If the test requested a pause at `initiate copy complete job' hook
 *          remove it now.                        
 *
 * @param   rg_config_p - Current raid group config
 * @param   raid_group_count - The index of the raid group arrays to set debug hook for
 * @param   position_to_copy - Raid group position that is being spared
 * @param   b_is_reboot_test - FBE_TRUE this is a reboot test
 *                             FBE_FALSE this is not a reboot test
 *
 * @return  fbe_status_t 
 *
 * @author
 *  11/14/2012  - Ron Proulx    - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_remove_initiate_copy_complete_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                   fbe_u32_t raid_group_count,
                                                                   fbe_u32_t position_to_copy,
                                                                   fbe_sep_package_load_params_t *sep_params_p,
                                                                   fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       rg_index;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                      b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /*! @note For dualsp and reboot don't set any hooks.
     */
    if ((b_is_dualsp_mode == FBE_TRUE) &&
        (b_is_reboot_test == FBE_TRUE)    )
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Both dualsp: %d and reboot: %d set - skip", 
                   __FUNCTION__, b_is_dualsp_mode, b_is_reboot_test);
        return FBE_STATUS_OK;
    }

    /* Only remove the rebuild hook if it is set.
     */
    if (fbe_test_sep_background_pause_b_is_initiate_copy_complete_hook_set == FBE_FALSE)
    {
        /* Rebuild hook not set.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Swap-out hook not set.", __FUNCTION__);
        return FBE_STATUS_OK;
    }

    /* Remove the swap-out hook for each virtual drive
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Remove the passed copy of the debug hook.
         */
        MUT_ASSERT_TRUE(rg_index < MAX_SCHEDULER_DEBUG_HOOKS);
        status = fbe_test_sep_background_pause_remove_debug_hook(&sep_params_p->scheduler_hooks[rg_index], rg_index,
                                                                 b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Flag the setting the private rebuild hook
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Remove swap-out hook raid obj: 0x%x pos: %d vd obj: 0x%x",
                   rg_object_id, position_to_copy, vd_object_id);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Clear the swap-out hook.
     */
    fbe_test_sep_background_pause_b_is_initiate_copy_complete_hook_set = FBE_FALSE;

    /* Return the status
     */
    return status;
}
/***********************************************************************
 * end fbe_test_sep_background_pause_remove_initiate_copy_complete_hook()
 ***********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_remove_swap_out_hook()
 *****************************************************************************
 *
 * @brief   If the test requested a pause at `swap-out' remove the hook now.                          
 *
 * @param   rg_config_p - Current raid group config
 * @param   raid_group_count - The index of the raid group arrays to set debug hook for
 * @param   position_to_copy - Raid group position that is being spared
 * @param   b_is_reboot_test - FBE_TRUE this is a reboot test
 *                             FBE_FALSE this is not a reboot test
 *
 * @return  fbe_status_t 
 *
 * @author
 *  06/02/2014  - Ron Proulx    - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_remove_swap_out_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                   fbe_u32_t raid_group_count,
                                                                   fbe_u32_t position_to_copy,
                                                                   fbe_sep_package_load_params_t *sep_params_p,
                                                                   fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       rg_index;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                      b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /*! @note For dualsp and reboot don't set any hooks.
     */
    if ((b_is_dualsp_mode == FBE_TRUE) &&
        (b_is_reboot_test == FBE_TRUE)    )
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Both dualsp: %d and reboot: %d set - skip", 
                   __FUNCTION__, b_is_dualsp_mode, b_is_reboot_test);
        return FBE_STATUS_OK;
    }

    /* Only remove the rebuild hook if it is set.
     */
    if (fbe_test_sep_background_pause_b_is_swap_out_complete_hook_set == FBE_FALSE)
    {
        /* Rebuild hook not set.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Swap-out hook not set.", __FUNCTION__);
        return FBE_STATUS_OK;
    }

    /* Remove the swap-out hook for each virtual drive
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Remove the passed copy of the debug hook.
         */
        MUT_ASSERT_TRUE(rg_index < MAX_SCHEDULER_DEBUG_HOOKS);
        status = fbe_test_sep_background_pause_remove_debug_hook(&sep_params_p->scheduler_hooks[rg_index], rg_index,
                                                                 b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Flag the setting the private rebuild hook
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Remove swap-out hook raid obj: 0x%x pos: %d vd obj: 0x%x",
                   rg_object_id, position_to_copy, vd_object_id);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Clear the swap-out hook.
     */
    fbe_test_sep_background_pause_b_is_swap_out_complete_hook_set = FBE_FALSE;

    /* Return the status
     */
    return status;
}
/***********************************************************************
 * end fbe_test_sep_background_pause_remove_swap_out_hook()
 ***********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_start_proactive_spare_drives()
 *****************************************************************************
 * @brief
 *  Forces drives into proactive sparing, one per raid group, then `pauses'
 *  once the copy operation reaches the percentage of raid group capacity
 *  requested.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_spare - raid group position to start proactive
 * @param   current_copy_state_p - Pointer to current copy state to update
 * @param   copy_state_to_pause_at - The copy state to set the debug hook for
 *              (if any)
 * @param   sep_params_p - Pointer to params containing debug hook information
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test (pause)
 *                             FBE_FALSE - This is not a reboot test (log)
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/22/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_start_proactive_spare_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                                               fbe_u32_t raid_group_count,
                                                                               fbe_u32_t position_to_spare,
                                                                               fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                                               fbe_test_sep_background_copy_state_t copy_state_to_pause_at,
                                                                               fbe_sep_package_load_params_t *sep_params_p,
                                                                               fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u8_t                        scsi_opcode_to_inject_on = FBE_SCSI_READ_16;
    fbe_bool_t                      b_start_io = FBE_TRUE;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_spare_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_object_id_t                 pvd_object_id = 0;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_protocol_error_injection_error_record_t error_record[FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT];
    fbe_protocol_error_injection_record_handle_t record_handle_p[FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT];

    /* Validate the `copy state to pause at'.
     */
    switch(copy_state_to_pause_at)
    {
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_NOT_APPLICABLE:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL:
            /* The above are the only allowable `pause at state' values.
             */
            break;

        default:
            /* This method doesn't support the request `pause at' state.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s: Unsupported `pause at' state: %d ",
                       __FUNCTION__, copy_state_to_pause_at);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
    }

    /* Set the rebuild hook so that we prevent the source drive from being swapped-out.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Set copy complete hook", __FUNCTION__);
    status = fbe_test_sep_background_pause_set_private_copy_complete_hook(rg_config_p, raid_group_count, position_to_spare,
                                                            sep_params_p, b_is_reboot_test);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Set copy complete - complete.", __FUNCTION__);

    /* For every raid group inject an error to the copy position
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* We need to add the drive to the `needing spare' position primarily for
         * debug purposes.
         */
        fbe_test_sep_util_add_needing_spare_position(current_rg_config_p, position_to_spare);
        drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_spare];

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_spare, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /*! @note Get the edge index of the source and destination drive based 
         *        on configuration mode.
         *
         *  @note This assumes that all raid groups have the same source and 
         *        destination index. 
         */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Save the source information.
         */
        status = fbe_test_sep_background_ops_save_source_info(current_rg_config_p, position_to_spare,
                                                              source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Start proactive on raid: 0x%x pos: %d vd obj: 0x%x(%d_%d_%d) source: %d dest: %d",
                   rg_object_id, position_to_spare, vd_object_id,
                   drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot,
                   source_edge_index, dest_edge_index);

        /* Get the provision drive object id for the position specified.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_spare_p->bus,
                                                                drive_to_spare_p->enclosure,
                                                                drive_to_spare_p->slot,
                                                                &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
        /* Validate the expected EOL state.
         */
        status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_FALSE(provision_drive_info.end_of_life_state);

        /* If enabled set debug hook.
         */
        if (copy_state_to_pause_at == FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL)
        {
            status = fbe_test_sep_background_pause_set_debug_hook(&sep_params_p->scheduler_hooks[rg_index], rg_index,
                                                            pvd_object_id,
                                                            SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EOL,
                                                            FBE_PROVISION_DRIVE_SUBSTATE_SET_EOL,
                                                            0, 0, 
                                                            SCHEDULER_CHECK_STATES,
                                                            SCHEDULER_DEBUG_ACTION_PAUSE,
                                                            b_is_reboot_test);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Populate the protocol error injection record.
         */
        status = fbe_test_sep_background_ops_populate_io_error_injection_record(current_rg_config_p,
                                                                      position_to_spare,
                                                                      scsi_opcode_to_inject_on,
                                                                      &error_record[rg_index],
                                                                      &record_handle_p[rg_index]);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Stop the job service recovery queue to hold the job service swap 
         * commands. 
         */
        fbe_test_sep_util_disable_recovery_queue(vd_object_id);

        /* Now allow automatic spare selection for this virtual drive object.
         */
        status = fbe_api_virtual_drive_set_unused_drive_as_spare_flag(vd_object_id, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Step 1:  Inject the scsi errors. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: start error injection.", __FUNCTION__);
    status = fbe_api_protocol_error_injection_start(); 
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);

    /*! @todo DE542 - Due to the fact that protocol error are injected on the
     *                way to the disk, the data is never sent to or read from
     *                the disk.  Therefore until this defect is fixed b_start_io
     *                MUST be True.
     */
    if (b_start_io == FBE_FALSE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Due to DE542 b_start_io: %d MUST be True !", 
                   __FUNCTION__, b_start_io);
    }
    MUT_ASSERT_TRUE(b_start_io == FBE_TRUE);

    /* If requested start I/O to trigger the smart error.
     */
    if (b_start_io == FBE_TRUE)
    {
        /* Start the background pattern to all the PVDs to trigger the 
         * errors. 
         */
        fbe_test_sep_background_ops_run_io_to_start_copy(rg_config_p,
                                                         raid_group_count,
                                                         position_to_spare,
                                                         scsi_opcode_to_inject_on, 
                                                         FBE_TEST_SEP_BACKGROUND_OPS_ELEMENT_SIZE);
    }

    /* stop injecting scsi errors. */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: stop error injection.", __FUNCTION__);
    status = fbe_api_protocol_error_injection_stop(); 
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(status == FBE_STATUS_OK);
	status  = fbe_api_protocol_error_injection_remove_all_records();
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    *current_copy_state_p = FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL;
    mut_printf(MUT_LOG_TEST_STATUS, "%s: stop error injection complete. state: %d", __FUNCTION__, *current_copy_state_p);

    /* If we are not waiting for EOL then re-enable the recovery queue.
     */
    if (copy_state_to_pause_at != FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL)
    {
        /* Step 2: Restart the job service queue to proceed with job service 
         * swap command. 
         */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                current_rg_config_p++;
                continue;
            }

            /* Fetch the drive to copy information from the position passed.
             */
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_spare];

            /* Get the vd object id of the position to force into proactive copy
             */
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_spare, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

            /* Display some infomation.
            */
            mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Enable recovery raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_spare, vd_object_id,
                   drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);

            /* Restart the job service queue to proceed with job service 
             * swap command. 
             */
            fbe_test_sep_util_enable_recovery_queue(vd_object_id);

            /* Goto next raid group.
             */
            current_rg_config_p++;

        } /* end for all raid groups*/

    } /* end if we don't need to pause at EOL */

    /* Return execution status.
     */
    return FBE_STATUS_OK;
}
/******************************************************************
 * end fbe_test_sep_background_pause_start_proactive_spare_drives()
 ******************************************************************/

/*!**************************************************************
 *  fbe_test_sep_background_pause_continue_proactive_spare_drives()
 ****************************************************************
 * @brief
 *  Continues the proactive sparing process after a `pause'.
 *
 * @param rg_config_p - Array of raid group information  
 * @param raid_group_count - Number of valid raid groups in array 
 * @param position_to_spare - raid group position to start proactive
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/26/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_test_sep_background_pause_continue_proactive_spare_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                                                  fbe_u32_t raid_group_count,
                                                                                  fbe_u32_t position_to_spare)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_spare_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_object_id_t                 pvd_object_id = 0;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;

    /* For every raid group get the copy position information
     */
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the drive to copy from the information passed
         */
        drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_spare];

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_spare, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /*! @note Get the edge index of the source and destination drive based 
         *        on configuration mode.
         *
         *  @note This assumes that all raid groups have the same source and 
         *        destination index. 
         */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Get the provision drive object id for the position specified.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_spare_p->bus,
                                                                drive_to_spare_p->enclosure,
                                                                drive_to_spare_p->slot,
                                                                &pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
        /* Validate the expected EOL state.
         */
        status = fbe_api_provision_drive_get_info(pvd_object_id, &provision_drive_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_TRUE(provision_drive_info.end_of_life_state);

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Continue proactive on raid: 0x%x pos: %d vd obj: 0x%x(%d_%d_%d) source: %d dest: %d",
                   rg_object_id, position_to_spare, vd_object_id,
                   drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot,
                   source_edge_index, dest_edge_index);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Step 1: Validate that PVD for the edge selected goes EOL
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for source edge EOL", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SOURCE_EDGE_EOL);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_spare,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Source edge EOL Complete.", __FUNCTION__);

    /* Step 2: Restart the job service queue to proceed with job service 
     * swap command. 
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Fetch the drive to copy information from the position passed.
         */
        drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_spare];

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_spare, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Display some infomation.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Continue - enable recovery raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_spare, vd_object_id,
                   drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);

        /* Restart the job service queue to proceed with job service 
         * swap command. 
         */
        fbe_test_sep_util_enable_recovery_queue(vd_object_id);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/*********************************************************************
 * end fbe_test_sep_background_pause_continue_proactive_spare_drives()
 *********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_start_user_copy()
 *****************************************************************************
 *
 * @brief   Initiates a user copy operation.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - The position in the raid group to copy from
 * @param   current_copy_state_p - Pointer to copy state to update
 * @param   sep_params_p - Pointer to reboot params
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test
 *
 * @return  fbe_status_t 
 *
 * @author
 *  04/09/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_start_user_copy(fbe_test_rg_configuration_t *rg_config_p,
                                                                            fbe_u32_t raid_group_count,
                                                                            fbe_u32_t position_to_copy,
                                                                            fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                                            fbe_sep_package_load_params_t *sep_params_p,
                                                                            fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_copy_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 source_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;
    fbe_provision_drive_copy_to_status_t copy_status;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Forcing proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Validate the destination array is not NULL
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Set the rebuild hook so that we prevent the source drive from being swapped-out.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Set copy complete hook", __FUNCTION__);
    status = fbe_test_sep_background_pause_set_private_copy_complete_hook(rg_config_p, raid_group_count, position_to_copy,
                                                            sep_params_p, b_is_reboot_test);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Set copy complete - complete.", __FUNCTION__);

    /* Step 1: Issue the user copy request (recovery queue is enabled).
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* We need to add the drive to the `needing spare' position primarily for
         * debug purposes.
         */
        fbe_test_sep_util_add_needing_spare_position(current_rg_config_p, position_to_copy);
        drive_to_copy_p = &current_rg_config_p->rg_disk_set[position_to_copy];

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /*! @note Get the edge index of the source and destination drive based 
         *        on configuration mode.
         *
         *  @note This assumes that all raid groups have the same source and 
         *        destination index. 
         */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Save the source information.
         */
        status = fbe_test_sep_background_ops_save_source_info(current_rg_config_p, position_to_copy,
                                                              source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: Start user copy on raid: 0x%x pos: %d vd obj: 0x%x(%d_%d_%d) source: %d dest: %d(n/a)",
                   rg_object_id, position_to_copy, vd_object_id,
                   drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot,
                   source_edge_index, dest_edge_index);

        /*  Get and validate the source and destination pvd object ids.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot,
                                                                &source_pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(0, source_pvd_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, source_pvd_object_id);

        /* Now allow automatic spare selection for this virtual drive object.
         */
        status = fbe_api_virtual_drive_set_unused_drive_as_spare_flag(vd_object_id, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /*! @note Now issue the proactive user copy request.  This API has
         *        no timeout so there must be spares available.
         */
        status = fbe_api_provision_drive_user_copy(source_pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR, copy_status);

        /* Goto next raid group and destination drive.
         */
        current_rg_config_p++;
    }

    /* Step 2: Wait for the spare to swap-in. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for Swap In", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_IN);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_copy,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    *current_copy_state_p = FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_COMPLETE;
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Swap In Complete. state: %d", __FUNCTION__, *current_copy_state_p);

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/***************************************************************
 * end fbe_test_sep_background_pause_start_user_copy()
 ***************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_start_user_copy_to()
 *****************************************************************************
 *
 * @brief   Initiates a user copy operation.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - The position in the raid group to copy from
 * @param   current_copy_state_p - Pointer to state to update
 * @param   sep_params_p - Pointer to reboot params
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test
 * @param   dest_array_p - Destination pvd object id for user copy operations
 *
 * @return  fbe_status_t 
 *
 * @author
 *  04/04/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_start_user_copy_to(fbe_test_rg_configuration_t *rg_config_p,
                                                                fbe_u32_t raid_group_count,
                                                                fbe_u32_t position_to_copy,
                                                                fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                                fbe_sep_package_load_params_t *sep_params_p,
                                                                fbe_bool_t b_is_reboot_test,
                                                                fbe_test_raid_group_disk_set_t *dest_array_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_copy_p = NULL;
    fbe_test_raid_group_disk_set_t *dest_drive_p = dest_array_p;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 source_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 dest_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;
    fbe_provision_drive_copy_to_status_t copy_status;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Forcing proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Validate that there aren't too many raid groups
     */
    if (dest_array_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: dest_array_p is NULL. Must supply destination for copy.", 
                   __FUNCTION__);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Validate the destination array is not NULL
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Set the rebuild hook so that we prevent the source drive from being swapped-out.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Set copy complete hook", __FUNCTION__);
    status = fbe_test_sep_background_pause_set_private_copy_complete_hook(rg_config_p, raid_group_count, position_to_copy,
                                                            sep_params_p, b_is_reboot_test);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Set copy complete hook - complete.", __FUNCTION__);

    /* Step 1: Issue the user copy request (recovery queue is enabled).
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* We need to add the drive to the `needing spare' position primarily for
         * debug purposes.
         */
        fbe_test_sep_util_add_needing_spare_position(current_rg_config_p, position_to_copy);
        drive_to_copy_p = &current_rg_config_p->rg_disk_set[position_to_copy];

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /*! @note Get the edge index of the source and destination drive based 
         *        on configuration mode.
         *
         *  @note This assumes that all raid groups have the same source and 
         *        destination index. 
         */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Save the source information.
         */
        status = fbe_test_sep_background_ops_save_source_info(current_rg_config_p, position_to_copy,
                                                              source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_ops: Start user copy on raid: 0x%x pos: %d vd obj: 0x%x(%d_%d_%d) source: %d dest: %d(%d_%d_%d)",
                   rg_object_id, position_to_copy, vd_object_id,
                   drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot,
                   source_edge_index, dest_edge_index,
                   dest_drive_p->bus, dest_drive_p->enclosure, dest_drive_p->slot);

        /*  Get and validate the source and destination pvd object ids.
         */
        status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot,
                                                                &source_pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(0, source_pvd_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, source_pvd_object_id);
        status = fbe_api_provision_drive_get_obj_id_by_location(dest_drive_p->bus, dest_drive_p->enclosure, dest_drive_p->slot,
                                                                &dest_pvd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(0, dest_pvd_object_id);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, dest_pvd_object_id);

        /* Now allow automatic spare selection for this virtual drive object.
         */
        status = fbe_api_virtual_drive_set_unused_drive_as_spare_flag(vd_object_id, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now issue the user copy request.
         */
        status = fbe_api_copy_to_replacement_disk(source_pvd_object_id, dest_pvd_object_id, &copy_status);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_EQUAL(FBE_PROVISION_DRIVE_COPY_TO_STATUS_NO_ERROR, copy_status);

        /* Goto next raid group and destination drive.
         */
        current_rg_config_p++;
        dest_drive_p++;
    }

    /* Step 2: Wait for the spare to swap-in. 
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for Swap In", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_IN);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_copy,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    *current_copy_state_p = FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_COMPLETE;
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Swap In Complete. state: %d", __FUNCTION__, *current_copy_state_p);

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_test_sep_background_pause_start_user_copy_to()
 **********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_start_copy()
 *****************************************************************************
 *
 * @brief   Start the requested copy operation.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to copy for
 * @param   copy_type - Type of copy operation requested
 * @param   current_copy_state_p - Address of current copy state to update
 * @param   dest_array_p - Pointer to array of destination drives for Copy To
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/27/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_start_copy(fbe_test_rg_configuration_t *rg_config_p,
                                                             fbe_u32_t raid_group_count,
                                                             fbe_u32_t position_to_copy,
                                                             fbe_spare_swap_command_t copy_type,
                                                             fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                             fbe_sep_package_load_params_t *sep_params_p,
                                                             fbe_bool_t b_is_reboot_test,
                                                             fbe_test_raid_group_disk_set_t *dest_array_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;

    /* Base on the copy request type, initiate the copy request.
     */
    switch(copy_type)
    {
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
            /* We should have already initiated the proactive copy.
             */
            break;
    
        case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
            /* Initiate the user copy command.
             */ 
            status = fbe_test_sep_background_pause_start_user_copy(rg_config_p, raid_group_count, position_to_copy,
                                                                             current_copy_state_p, sep_params_p, b_is_reboot_test);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    
        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            /* Initiate the user copy to request.
             */
            status = fbe_test_sep_background_pause_start_user_copy_to(rg_config_p, raid_group_count, position_to_copy,
                                                                   current_copy_state_p, sep_params_p, b_is_reboot_test,
                                                                   dest_array_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    
        default:
            /* Unsupported copy command type.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s - Unexpected current copy type: %d ",
                       __FUNCTION__, copy_type);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
    }

    return status;

}
/**********************************************************
 * end fbe_test_sep_background_pause_start_copy()
 **********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_copy_started()
 *****************************************************************************
 *
 * @brief   Wait for the copy to reached the desired point:
 *              o Copy start (pause after metadata is rebuilt)
 *              o Copy has reached the desired rebuild percentage
 *              o Copy complete
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to start proactive
 * @param   current_copy_state_p - Address of current copy state to update
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/27/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_copy_started(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position_to_copy,
                                                               fbe_test_sep_background_copy_state_t *current_copy_state_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_copy_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;

    /* First loop is simply to print a message
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* The position to complete is supplied.
         */
        drive_to_copy_p = &current_rg_config_p->rg_disk_set[position_to_copy];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Get the edge index of the source and destination drive based on configuration mode. */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Wait for copy start on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_copy, vd_object_id,
                   drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot);

        /* Goto the next raid group
         */
        current_rg_config_p++;
    }

    /* Step 1: Wait for the copy operation to start
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for virtual drive copy start", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_START);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_copy,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    *current_copy_state_p = FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_START;
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Virtual drive copy started. state: %d", __FUNCTION__, *current_copy_state_p);

    /* Return the execution status.
     */
    return status;
}
/*****************************************************************
 * end fbe_test_sep_background_pause_copy_started()
 *****************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_copy_complete()
 *****************************************************************************
 *
 * @brief   Wait for the copy to be compeleted.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to start proactive
 * @param   current_copy_state_p - Address of current copy state to update
 * @param   sep_params_p - Pointer to sep parameters to update
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test.
 *                             FBE_FALSE - This is not a reboot test.
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/27/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_copy_complete(fbe_test_rg_configuration_t *rg_config_p,
                                                                fbe_u32_t raid_group_count,
                                                                fbe_u32_t position_to_copy,
                                                                fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                                fbe_sep_package_load_params_t *sep_params_p,
                                                                fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_copy_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;

    /* First loop is simply to print a message
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* The position to complete is supplied.
         */
        drive_to_copy_p = &current_rg_config_p->rg_disk_set[position_to_copy];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Get the edge index of the source and destination drive based on configuration mode. */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Wait for copy complete on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_copy, vd_object_id,
                   drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot);

        /* Goto the next raid group
         */
        current_rg_config_p++;
    }


    /* Step 1: Wait for all chunks on the virtual drive to be rebuilt.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for virtual drive copy complete", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_COMPLETE);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_copy,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    *current_copy_state_p = FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_COMPLETE;
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Virtual drive copy complete. state: %d", __FUNCTION__, *current_copy_state_p);

    /* Step 2: Remove the rebuild hook that prevented source drive from being swapped-out.
     */
    if (fbe_test_sep_background_pause_b_is_private_rebuild_hook_set == FBE_TRUE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Remove copy complete hook", __FUNCTION__);
        status = fbe_test_sep_background_pause_remove_private_copy_complete_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                   sep_params_p, b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Remove copy complete hook - complete.", __FUNCTION__);
    }

    /* Return the execution status.
     */
    return status;
}
/*****************************************************************
 * end fbe_test_sep_background_pause_copy_complete()
 *****************************************************************/

/*!***************************************************************************
 * fbe_test_sep_background_pause_copy_cleanup_reinsert_drive_if_required()
 *****************************************************************************
 * @brief
 *  insert drives, one per raid group.
 * 
 * @param rg_config_p - raid group config array.
 * @param raid_group_count
 * @param position_to_copy - The position being copied
 * @param b_skip_cleanup_p - Pointer to bool indicating if we 
 *
 * @return None.
 *
 * @author
 *  07/19/2012   Ron Proulx - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_copy_cleanup_reinsert_drive_if_required(fbe_test_rg_configuration_t *rg_config_p,
                                                                                          fbe_u32_t raid_group_count,
                                                                                          fbe_u32_t position_to_copy,
                                                                                          fbe_bool_t b_clear_eol,
                                                                                          fbe_bool_t *b_skip_cleanup_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   rg_index;
    fbe_u32_t                   removed_index;
    fbe_test_raid_group_disk_set_t *drive_to_insert_p = NULL;
    fbe_object_id_t             pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t             pdo_object_id = FBE_OBJECT_ID_INVALID;
    fbe_test_rg_configuration_t *current_rg_config_p = rg_config_p;
    fbe_u32_t                   position_to_insert;
    fbe_bool_t                  b_object_removed = FBE_FALSE;
    fbe_api_base_config_downstream_object_list_t downstream_object_list; 

    /* By default don't skip cleanup.
     */
    *b_skip_cleanup_p = FBE_FALSE;

    /* For every raid group insert one drive.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        fbe_u32_t wait_attempts = 0;

        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* First checked if the drive is removed or not.
         */
        for (removed_index = 0; removed_index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; removed_index++)
        {
            if (current_rg_config_p->drives_removed_array[removed_index] == position_to_copy)
            {
                b_object_removed = FBE_TRUE;
                position_to_insert = position_to_copy;
                break;
            }
        }

        /* If the source position was removed, re-insert it.
         */
        if (b_object_removed == FBE_TRUE)
        {
            fbe_test_sep_util_removed_position_inserted(current_rg_config_p, position_to_insert);
            drive_to_insert_p = &current_rg_config_p->source_disk;
    
            mut_printf(MUT_LOG_TEST_STATUS, "== %s inserting rg %d position %d (%d_%d_%d). ==", 
                       __FUNCTION__, rg_index, position_to_insert, 
                       drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot);

            /* inserting the same drive back */
            if(fbe_test_util_is_simulation())
            {
                status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                     drive_to_insert_p->enclosure, 
                                                     drive_to_insert_p->slot,
                                                     current_rg_config_p->drive_handle[position_to_insert]);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                if (fbe_test_sep_util_get_dualsp_test_mode())
                {
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                    status = fbe_test_pp_util_reinsert_drive(drive_to_insert_p->bus, 
                                                             drive_to_insert_p->enclosure, 
                                                             drive_to_insert_p->slot,
                                                             current_rg_config_p->peer_drive_handle[position_to_insert]);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
                }
            }
            else
            {
                status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                     drive_to_insert_p->enclosure, 
                                                     drive_to_insert_p->slot);
                MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
                if (fbe_test_sep_util_get_dualsp_test_mode())
                {
                    /* Set the target server to SPB and insert the drive there. */
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);
                    status = fbe_test_pp_util_reinsert_drive_hw(drive_to_insert_p->bus, 
                                                                drive_to_insert_p->enclosure, 
                                                                drive_to_insert_p->slot);
                    MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
    
                    /* Set the target server back to SPA. */
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);
                }
            }

            /* Wait for object to be valid.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for rg %d position %d (%d_%d_%d) to become ready ==", 
                       __FUNCTION__, rg_index, position_to_insert, 
                       drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot);
            status = FBE_STATUS_GENERIC_FAILURE;
            while ((status != FBE_STATUS_OK) &&
                   (wait_attempts < 20)         )
            {
                /* Wait until we can get pvd object id.
                 */
                status = fbe_api_provision_drive_get_obj_id_by_location(drive_to_insert_p->bus,
                                                                    drive_to_insert_p->enclosure,
                                                                    drive_to_insert_p->slot,
                                                                    &pvd_object_id);
                if (status != FBE_STATUS_OK)
                {
                    fbe_api_sleep(500);
                }
            }
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Wait for the pdo to become ready
             */
            downstream_object_list.number_of_downstream_objects = 0;
            while ((downstream_object_list.number_of_downstream_objects != 1) &&
                   (wait_attempts < 20)                                          )
            {
                /* Wait until we detect the downstream pdo
                 */
                status = fbe_api_base_config_get_downstream_object_list(pvd_object_id, &downstream_object_list);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                if (downstream_object_list.number_of_downstream_objects != 1)
                {
                    fbe_api_sleep(500);
                }
            }
            MUT_ASSERT_INT_EQUAL(1, downstream_object_list.number_of_downstream_objects);
            pdo_object_id = downstream_object_list.downstream_object_list[0];
            status = fbe_api_wait_for_object_lifecycle_state(pdo_object_id, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_PHYSICAL);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* If dualsp wait for pdo Ready on peer.
             */
            if (fbe_test_sep_util_get_dualsp_test_mode())
            {
                    /* Set the target server to SPB and insert the drive there. */
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_B);

                    downstream_object_list.number_of_downstream_objects = 0;
                    while ((downstream_object_list.number_of_downstream_objects != 1) &&
                           (wait_attempts < 20)                                          )
                    {
                        /* Wait until we detect the downstream pdo
                         */
                        status = fbe_api_base_config_get_downstream_object_list(pvd_object_id, &downstream_object_list);
                        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                        if (downstream_object_list.number_of_downstream_objects != 1)
                        {
                            fbe_api_sleep(500);
                        }
                    }
                    MUT_ASSERT_INT_EQUAL(1, downstream_object_list.number_of_downstream_objects);
                    pdo_object_id = downstream_object_list.downstream_object_list[0];
                    status = fbe_api_wait_for_object_lifecycle_state(pdo_object_id, FBE_LIFECYCLE_STATE_READY, 10000, FBE_PACKAGE_ID_PHYSICAL);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                    /* Set the target server back to SPA. */
                    fbe_api_sim_transport_set_target_server(FBE_SIM_SP_A);

            } /* end if dualsp need to wait for pdo Ready on peer*/

            /* If required (i.e. proactive copy), call method to clear
             * EOL in the drive.  Eventually all upstream edges should have the
             * EOL flag cleared.
             */
            if (b_clear_eol == FBE_TRUE)
            {
                status = fbe_test_sep_util_clear_disk_eol(drive_to_insert_p->bus,
                                                          drive_to_insert_p->enclosure,
                                                          drive_to_insert_p->slot);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                if (status != FBE_STATUS_OK)
                {
                    return status;
                }
            }

            status = fbe_api_wait_for_object_lifecycle_state(pvd_object_id, FBE_LIFECYCLE_STATE_READY, 70000, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            mut_printf(MUT_LOG_TEST_STATUS, "== %s Wait for %d_%d_%d to become ready - complete ==", 
                       __FUNCTION__,
                       drive_to_insert_p->bus, drive_to_insert_p->enclosure, drive_to_insert_p->slot);
        }
        else
        {
            /* Else we need to clear EOL.
             */
            if (b_clear_eol == FBE_TRUE)
            {
                status = fbe_test_sep_util_clear_disk_eol(current_rg_config_p->source_disk.bus,
                                                          current_rg_config_p->source_disk.enclosure,
                                                          current_rg_config_p->source_disk.slot);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                if (status != FBE_STATUS_OK)
                {
                    return status;
                }
            }
        }

        /* Goto next raid group
        */
        current_rg_config_p++;
    }

    return FBE_STATUS_OK;
}
/*****************************************************************************
 * end fbe_test_sep_background_pause_copy_cleanup_reinsert_drive_if_required()
 *****************************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_is_source_degraded()
 *****************************************************************************
 *
 * @brief   This method checks if any of the copy virtual drive is degraded
 *          (i.e. the source drive rebuild checkpoint is not at the end  marker).
 *          This can occur when the source drive is removed during a copy
 *          operation and the parent raid group is in the process of marking
 *          NR for the chunks that were not copied.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to start proactive
 *
 * @return  bool - FBE_TRUE - At least (1) of the virtual drives is degraded
 *                 FBE_FALSE - None of the virtual drives are degraded.
 *
 * @author
 *  10/12/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_bool_t fbe_test_sep_background_pause_is_source_degraded(fbe_test_rg_configuration_t *rg_config_p,
                                                                   fbe_u32_t raid_group_count,
                                                                   fbe_u32_t position_to_copy)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t  source_drive_location;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_api_virtual_drive_get_info_t vd_get_info;
    fbe_object_id_t                 pvd_object_id = 0;
    fbe_api_raid_group_get_info_t   raid_group_info;
    fbe_lba_t                       vd_source_drive_rebuild_checkpoint = 0;

    /* For every raid group get the copy position information
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /*! @note We have already swapped-out so th destination edge should be invalid
         *
         *  @note This assumes that all raid groups have the same source and 
         *        destination index. 
         */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Get the provision drive object id for source drive of the virtual drive
         */
        status = fbe_api_virtual_drive_get_info(vd_object_id, &vd_get_info);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        pvd_object_id = vd_get_info.original_pvd_object_id;

        /* Get the location of the new source drive.
         */
        status = fbe_api_provision_drive_get_location(pvd_object_id,
                                                      &source_drive_location.bus,
                                                      &source_drive_location.enclosure,
                                                      &source_drive_location.slot);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        
        /* Now get the virtual drive raid group information
         */
        status = fbe_api_raid_group_get_info(vd_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Now get the rebuild checkpoint for the source position.
         */
        vd_source_drive_rebuild_checkpoint = raid_group_info.rebuild_checkpoint[source_edge_index];
        if (vd_source_drive_rebuild_checkpoint != FBE_LBA_INVALID)
        {
            /* Display some information
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Check source degraded raid: 0x%x pos: %d vd obj: 0x%x(%d_%d_%d) source: %d chkpt: 0x%llx",
                   rg_object_id, position_to_copy, vd_object_id,
                   source_drive_location.bus, source_drive_location.enclosure, source_drive_location.slot,
                   source_edge_index, vd_source_drive_rebuild_checkpoint);

            /* Return `True' (there is at least (1) virtual drive that has a 
             * degraded source position.
             */
            return FBE_TRUE;
        }

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* If none of the virtual drives are degraded return `False'
     */
    return FBE_FALSE;
}
/*********************************************************************
 * end fbe_test_sep_background_pause_is_source_degraded()
 *********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_copy_cleanup()
 *****************************************************************************
 *
 * @brief   Perform any cleanup required.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to start proactive
 * @param   current_copy_state_p - Address of current copy state to update
 * @param   sep_params_p - Pointer to sep parameters to update
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test.
 *                             FBE_FALSE - This is not a reboot test.
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/27/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_copy_cleanup(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position_to_copy,
                                                               fbe_spare_swap_command_t copy_type,
                                                               fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                               fbe_sep_package_load_params_t *sep_params_p,
                                                               fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_copy_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;
    fbe_bool_t                      b_skip_cleanup = FBE_FALSE;
    fbe_bool_t                      b_clear_eol = FBE_FALSE;

    /* First loop is simply to print a message
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* The position to complete is supplied.
         */
        drive_to_copy_p = &current_rg_config_p->rg_disk_set[position_to_copy];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Get the edge index of the source and destination drive based on configuration mode. */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Wait for copy cleanup on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_copy, vd_object_id,
                   drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot);

        /* Goto the next raid group
         */
        current_rg_config_p++;
    }

    /* Step 1: Remove the rebuild hook that prevented source drive from being swapped-out.
     */
    if (fbe_test_sep_background_pause_b_is_private_rebuild_hook_set == FBE_TRUE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Remove copy complete hook", __FUNCTION__);
        status = fbe_test_sep_background_pause_remove_private_copy_complete_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                   sep_params_p, b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Remove copy complete hook - complete.", __FUNCTION__);
    }

    /* Step 2: Remove the initiate copy complte job hook that prevented source drive from being swapped-out.
     */
    if (fbe_test_sep_background_pause_b_is_initiate_copy_complete_hook_set == FBE_TRUE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Remove initiate copy complete hook", __FUNCTION__);
        status = fbe_test_sep_background_pause_remove_initiate_copy_complete_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                   sep_params_p, b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Remove initiate copy complete - complete.", __FUNCTION__);
    }

    /* Step 3: Remove the swap-out hook that prevented source drive from being swapped-out.
     */
    if (fbe_test_sep_background_pause_b_is_swap_out_complete_hook_set == FBE_TRUE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Remove swap-out hook", __FUNCTION__);
        status = fbe_test_sep_background_pause_remove_swap_out_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                   sep_params_p, b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Remove swap-out hook - complete.", __FUNCTION__);
    }

    /* Step 4: Wait for source to be swapped out of virtual drive
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for source to be swapped out", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_OUT);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_copy,
                                                        event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    *current_copy_state_p = FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_SWAP_OUT_COMPLETE;
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Source swap out complete. state: %d", __FUNCTION__, *current_copy_state_p);

    /* Step 5: If there source drive was removed we need to wait until the 
     *         parent raid group set the checkpoint to the end marker.
     */
    if (fbe_test_sep_background_pause_is_source_degraded(rg_config_p, raid_group_count, position_to_copy))
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for degraded source to be marked NR", __FUNCTION__);
        event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SOURCE_MARKED_NR);
        status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_copy,
                                                            event_info_p, source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for degraded source to be marked NR - complete. state: %d", __FUNCTION__, *current_copy_state_p);
    }

    /* Step 6: Re-insert removed drives if required.
     */
    if (copy_type == FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND)
    {
        b_clear_eol = FBE_TRUE;
    }
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Re-insert any removed disks", __FUNCTION__);
    status = fbe_test_sep_background_pause_copy_cleanup_reinsert_drive_if_required(rg_config_p, raid_group_count, position_to_copy,
                                                                                   b_clear_eol, /* Clear EOL if required */
                                                                                   &b_skip_cleanup);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Re-insert any removed disks skip cleanup: %d", __FUNCTION__, b_skip_cleanup);

    /*! @note If cleanup worked
     */
    if (b_skip_cleanup == FBE_FALSE)
    {
        /* Step 7: Cleanup the disks
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Copy Cleanup disks", __FUNCTION__);
        status = fbe_test_sep_background_ops_copy_cleanup_disks(rg_config_p, raid_group_count, position_to_copy,
                                                                FBE_FALSE /* EOL cleared above */);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Copy Cleanup disks complete.", __FUNCTION__);
    
        /* Step 8: Re-insert/cleanup failed drives.
         */
        if (copy_type == FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND)
        {
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for swapped-out drive to be re-inserted", __FUNCTION__);
            event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_REINSERT_FAILED_DRIVES);
            status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_copy,
                                                                event_info_p, source_edge_index, dest_edge_index);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Swapped-out drives re-inserted", __FUNCTION__);
        }
    }
    else
    {
        /*! @todo Need to get drive cleanup working*/
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Skipping cleanup of failed drive TODO: Fix this!", __FUNCTION__);
    }

    /* Step 7: In all cases we need to `refresh' the actual drives being used in each
     *         of the raid group test configurations.
     */
    status = fbe_test_sep_background_ops_refresh_raid_group_disk_info(rg_config_p, raid_group_count);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    *current_copy_state_p = FBE_TEST_SEP_BACKGROUND_COPY_STATE_COPY_OPERATION_COMPLETE;

    /* Return the execution status.
     */
    return status;
}
/*****************************************************************
 * end fbe_test_sep_background_pause_copy_cleanup()
 *****************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_wait_for_swap_in()
 *****************************************************************************
 *
 * @brief   After the copy operation has started this method waits for the
 *          swap-in of the destination drive to complete.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to start proactive
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/22/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_wait_for_swap_in(fbe_test_rg_configuration_t *rg_config_p,
                                                                   fbe_u32_t raid_group_count,
                                                                   fbe_u32_t position_to_copy)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_copy_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;

    /* First loop is simply to print a message
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* The position to complete is supplied.
         */
        drive_to_copy_p = &current_rg_config_p->rg_disk_set[position_to_copy];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Get the edge index of the source and destination drive based on configuration mode. */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Wait for swap-in on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_copy, vd_object_id,
                   drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot);

        /* Goto the next raid group
         */
        current_rg_config_p++;
    }

    /* Step 1: Note the debug hook will prevent the swap-in so there
     *         is nothing to do.
     */

    /* Return the execution status.
     */
    return status;
}
/*****************************************************************
 * end fbe_test_sep_background_pause_wait_for_swap_in()
 *****************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_wait_for_copy_start()
 *****************************************************************************
 *
 * @brief   Wait for the copy to reached the desired point:
 *              o Copy start (pause after metadata is rebuilt)
 *              o Copy has reached the desired rebuild percentage
 *              o Copy complete
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to start proactive
 * @param   current_copy_state_p - Address of current copy state to update
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/27/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_wait_for_copy_start(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position_to_copy,
                                                               fbe_test_sep_background_copy_state_t *current_copy_state_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_copy_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;

    /* First loop is simply to print a message
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* The position to complete is supplied.
         */
        drive_to_copy_p = &current_rg_config_p->rg_disk_set[position_to_copy];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Get the edge index of the source and destination drive based on configuration mode. */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Wait for copy start on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_copy, vd_object_id,
                   drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot);

        /* Goto the next raid group
         */
        current_rg_config_p++;
    }

    /* Step 1: Wait for the copy operation to start (unless the `rebuild start 
     *         hook' is set.
     */
    if (fbe_test_sep_background_pause_b_is_rebuild_start_hook_set == FBE_FALSE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for virtual drive copy start", __FUNCTION__);
        event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_START);
        status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_copy,
                                                  event_info_p, source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        *current_copy_state_p = FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_START;
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Virtual drive copy started. state: %d", __FUNCTION__, *current_copy_state_p);
    }
    else
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Rebuild metadata hook set", __FUNCTION__);
         *current_copy_state_p = FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_START;
    }

    /* Return the execution status.
     */
    return status;
}
/*****************************************************************
 * end fbe_test_sep_background_pause_wait_for_copy_start()
 *****************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_wait_for_copy_complete()
 *****************************************************************************
 *
 * @brief   Wait for the copy to be completed.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to start proactive
 * @param   current_copy_state_p - Address of current copy state to update
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/27/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_wait_for_copy_complete(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position_to_copy,
                                                               fbe_test_sep_background_copy_state_t *current_copy_state_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_copy_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;

    /* First loop is simply to print a message
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* The position to complete is supplied.
         */
        drive_to_copy_p = &current_rg_config_p->rg_disk_set[position_to_copy];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Get the edge index of the source and destination drive based on configuration mode. */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Wait for copy complete on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_copy, vd_object_id,
                   drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot);

        /* Goto the next raid group
         */
        current_rg_config_p++;
    }

    /* Step 1: Wait for all chunks on the virtual drive to be rebuilt.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for virtual drive copy complete", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_COMPLETE);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_copy,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    *current_copy_state_p = FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_COMPLETE;
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Virtual drive copy complete. state: %d", __FUNCTION__, *current_copy_state_p);

    /* Return the execution status.
     */
    return status;
}
/*****************************************************************
 * end fbe_test_sep_background_pause_wait_for_copy_complete()
 *****************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_wait_for_initiate_copy_complete_job()
 *****************************************************************************
 *
 * @brief   Wait for the source swap-out to begin (we don't wait for the 
 *          swap-out to complete).
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to start proactive
 * @param   current_copy_state_p - Address of current copy state to update
 * @param   sep_params_p - Pointer to params (hooks)
 * @param   b_is_reboot_test - Is reboot test or not
 *
 * @return fbe_status_t 
 *
 * @author
 *  07/06/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_wait_for_initiate_copy_complete_job(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position_to_copy,
                                                               fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                               fbe_sep_package_load_params_t *sep_params_p,
                                                               fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_copy_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;

    /* First loop is simply to print a message
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* The position to complete is supplied.
         */
        drive_to_copy_p = &current_rg_config_p->rg_disk_set[position_to_copy];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Get the edge index of the source and destination drive based on configuration mode. */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Wait for initiate copy complete start on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_copy, vd_object_id,
                   drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot);

        /* Goto the next raid group
         */
        current_rg_config_p++;
    }

    /* Step 1: All we can do is validate that the current state is `rebuild complete'
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for copy complete hook", __FUNCTION__);
    status = fbe_test_sep_background_pause_wait_and_clear_private_copy_complete_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                 sep_params_p, b_is_reboot_test);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for copy complete hook - complete.", __FUNCTION__);
    *current_copy_state_p = FBE_TEST_SEP_BACKGROUND_COPY_STATE_INITIATE_COPY_COMPLETE_JOB;

    /* Return the execution status.
     */
    return status;
}
/*****************************************************************
 * end fbe_test_sep_background_pause_wait_for_initiate_copy_complete_job()
 *****************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_wait_for_swap_out_complete_start()
 *****************************************************************************
 *
 * @brief   Wait for the source swap-out to begin (we don't wait for the 
 *          swap-out to complete).
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to start proactive
 * @param   current_copy_state_p - Address of current copy state to update
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/02/2014  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_wait_for_swap_out_complete_start(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position_to_copy,
                                                               fbe_test_sep_background_copy_state_t *current_copy_state_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_copy_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;

    /* First loop is simply to print a message
     */
    current_rg_config_p = rg_config_p;
    for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* The position to complete is supplied.
         */
        drive_to_copy_p = &current_rg_config_p->rg_disk_set[position_to_copy];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Get the edge index of the source and destination drive based on configuration mode. */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Wait for swap-out complete start on raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                   rg_object_id, position_to_copy, vd_object_id,
                   drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot);

        /* Goto the next raid group
         */
        current_rg_config_p++;
    }

    /* Step 1: Wait for the `initiate copy complete' request-in-progress.
     */
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for swap-out complete start", __FUNCTION__);
    event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_COPY_COMPLETE_INITIATED);
    status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_copy,
                                              event_info_p, source_edge_index, dest_edge_index);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    *current_copy_state_p = FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_PASS_THRU;
    mut_printf(MUT_LOG_TEST_STATUS, "%s: Virtual drive copy complete start. state: %d", __FUNCTION__, *current_copy_state_p);

    /* Return the execution status.
     */
    return status;
}
/**********************************************************************
 * end fbe_test_sep_background_pause_wait_for_swap_out_complete_start()
 **********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_copy_pause_after_eol()
 *****************************************************************************
 * @brief
 *  Forces drives into proactive sparing, one per raid group, then `pauses'
 *  once the copy operation marks End-Of-Life on the source provision drive.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_spare - raid group position to start proactive
 * @param   current_copy_state_p - Address of current copy state to update
 * @param   copy_state_to_pause_at - The copy state to `pause' at
 * @param   validation_event - Event to validate (`None' indicates no validation)
 * @param   sep_params_p - Pointer to sep parameters to update
 * @param   percent_copied_before_pause - Percentage of raid group capacity that
 *              is copied before we `pause' (add a switch hook).
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test.
 *                             FBE_FALSE - This is not a reboot test.
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/22/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_copy_pause_after_eol(fbe_test_rg_configuration_t *rg_config_p,
                                                                 fbe_u32_t raid_group_count,
                                                                 fbe_u32_t position_to_spare,
                                                                 fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                                 fbe_test_sep_background_copy_event_t validation_event,
                                                                 fbe_sep_package_load_params_t *sep_params_p,
                                                                 fbe_bool_t b_is_reboot_test)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;

    /* If we are just valid
     */
    switch(validation_event)
    {
        case FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE:
            /* Based on the current state take the actions necessary.
             */
            switch(*current_copy_state_p)
            {
                case FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED:
                case FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_STARTED:
                    status = fbe_test_sep_background_pause_start_proactive_spare_drives(rg_config_p, raid_group_count, position_to_spare,
                                                                                        current_copy_state_p,
                                                                                        FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL,
                                                                                        sep_params_p, b_is_reboot_test);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    break;
    
                default:
                    /* All other cases are unexpected.
                     */
                    status = FBE_STATUS_GENERIC_FAILURE;
                    mut_printf(MUT_LOG_TEST_STATUS, 
                               "%s - Unexpected current copy state: %d ",
                               __FUNCTION__, *current_copy_state_p);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    return status;
            }
            break;

        case FBE_TEST_SEP_BACKGROUND_COPY_EVENT_SOURCE_MARKED_EOL:
            /* Step 1: Wait for the PVD to be marked EOL debug hook.
             */
            status = fbe_test_sep_background_pause_wait_for_and_remove_debug_hook(rg_config_p, raid_group_count, position_to_spare,
                                                                       sep_params_p, b_is_reboot_test, FBE_TRUE);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Step 2: Continue the proactive sparing process
             */
            status = fbe_test_sep_background_pause_continue_proactive_spare_drives(rg_config_p, raid_group_count, position_to_spare);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        default:
            /* event not supported.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s: Unsupported validation event: %d ",
                       __FUNCTION__, validation_event);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;

    }

    /* Return the execution status.
     */
    return status;
}
/****************************************************
 * end fbe_test_sep_background_copy_pause_after_eol()
 ****************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_copy_pause_after_swap_in()
 *****************************************************************************
 *
 * @brief   Forces drives into proactive sparing, one per raid group, then 
 *          `pauses' once the destination drive(s) have been swapped in.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to copy for
 * @param   copy_type - Type of copy operation requested
 * @param   current_copy_state_p - Address of current copy state to update
 * @param   copy_state_to_pause_at - The copy state to `pause' at
 * @param   validation_event - Event to validate (`None' indicates no validation)
 * @param   sep_params_p - Pointer to sep parameters to update
 * @param   percent_copied_before_pause - Percentage of raid group capacity that
 *              is copied before we `pause' (add a switch hook).
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test.
 *                             FBE_FALSE - This is not a reboot test.
 * @param   dest_array_p - Pointer to array of destination drives for Copy To
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/25/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_copy_pause_after_swap_in(fbe_test_rg_configuration_t *rg_config_p,
                                                                 fbe_u32_t raid_group_count,
                                                                 fbe_u32_t position_to_copy,
                                                                 fbe_spare_swap_command_t copy_type,
                                                                 fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                                 fbe_test_sep_background_copy_event_t validation_event,
                                                                 fbe_sep_package_load_params_t *sep_params_p,
                                                                 fbe_bool_t b_is_reboot_test,
                                                                 fbe_test_raid_group_disk_set_t *dest_array_p)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_spare_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;

    /* Now set the debug hook as required.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the drive to spare
         */
        drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /*! @note Get the edge index of the source and destination drive based 
         *        on configuration mode.
         *
         *  @note This assumes that all raid groups have the same source and 
         *        destination index. 
         */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Only set the debug hook if we are not validating that an event occurred.
         */
        if (validation_event == FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE)
        {
            /* Set the debug hook
             */
            status = fbe_test_sep_background_pause_set_debug_hook(&sep_params_p->scheduler_hooks[rg_index], rg_index,
                                                            vd_object_id,
                                                            SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN,
                                                            FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED,
                                                            0, 0, 
                                                            SCHEDULER_CHECK_STATES,
                                                            SCHEDULER_DEBUG_ACTION_PAUSE,
                                                            b_is_reboot_test);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }


    /* Base on the copy request type, initiate the copy request.
     */
    if (validation_event == FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE)
    {
        /* Start the copy request.
         */
        status = fbe_test_sep_background_pause_start_copy(rg_config_p, raid_group_count, position_to_copy,
                                                          copy_type, current_copy_state_p, sep_params_p,
                                                          b_is_reboot_test, dest_array_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* If we are just validating that a particular event wait now.
     */
    if (validation_event != FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for Swap In", __FUNCTION__);
        status = fbe_test_sep_background_pause_wait_for_and_remove_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                   sep_params_p, b_is_reboot_test, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_SWAP_IN);
        status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_copy,
                                                            event_info_p, source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        *current_copy_state_p = FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_COMPLETE;
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Swap In Complete. state: %d ", __FUNCTION__, *current_copy_state_p);
        return status;
    }

    /* Based on the current state take the actions necessary.
     */
    switch(*current_copy_state_p)
    {
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_STARTED:
            status = fbe_test_sep_background_pause_start_proactive_spare_drives(rg_config_p, raid_group_count, position_to_copy,
                                                                                current_copy_state_p,
                                                                                FBE_TEST_SEP_BACKGROUND_COPY_STATE_NOT_APPLICABLE,
                                                                                sep_params_p, b_is_reboot_test);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_START:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_COMPLETE:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_MIRROR:
            status = fbe_test_sep_background_pause_wait_for_swap_in(rg_config_p, raid_group_count, position_to_copy);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        default:
            /* All other cases are unexpected.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s - Unexpected current copy state: %d ",
                       __FUNCTION__, *current_copy_state_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* Return the execution status.
     */
    return status;
}
/********************************************************
 * end fbe_test_sep_background_copy_pause_after_swap_in()
 ********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_copy_pause_after_mirror_mode()
 *****************************************************************************
 *
 * @brief   Forces drives into proactive sparing, one per raid group, then 
 *          `pauses' once the virtual drive is set to mirror mode.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to copy for
 * @param   copy_type - Type of copy operation requested
 * @param   current_copy_state_p - Address of current copy state to update
 * @param   copy_state_to_pause_at - The copy state to `pause' at
 * @param   validation_event - Event to validate (`None' indicates no validation)
 * @param   sep_params_p - Pointer to sep parameters to update
 * @param   percent_copied_before_pause - Percentage of raid group capacity that
 *              is copied before we `pause' (add a switch hook).
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test.
 *                             FBE_FALSE - This is not a reboot test.
 * @param   dest_array_p - Pointer to array of destination drives for Copy To
 *
 * @return fbe_status_t 
 *
 * @author
 *  07/12/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_copy_pause_after_mirror_mode(fbe_test_rg_configuration_t *rg_config_p,
                                                                 fbe_u32_t raid_group_count,
                                                                 fbe_u32_t position_to_copy,
                                                                 fbe_spare_swap_command_t copy_type,
                                                                 fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                                 fbe_test_sep_background_copy_event_t validation_event,
                                                                 fbe_sep_package_load_params_t *sep_params_p,
                                                                 fbe_bool_t b_is_reboot_test,
                                                                 fbe_test_raid_group_disk_set_t *dest_array_p)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_spare_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;


    /* Now set the debug hook as required.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the drive to spare
         */
        drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /*! @note Get the edge index of the source and destination drive based 
         *        on configuration mode.
         *
         *  @note This assumes that all raid groups have the same source and 
         *        destination index. 
         */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Only set the debug hook if we are not validating that an event occurred.
         */
        if (validation_event == FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE)
        {
            /* Set the debug hook
             */
            status = fbe_test_sep_background_pause_set_debug_hook(&sep_params_p->scheduler_hooks[rg_index], rg_index,
                                                                  vd_object_id,
                                                                  SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY, 
                                                                  FBE_VIRTUAL_DRIVE_SUBSTATE_MIRROR_MODE_SET,
                                                                  0, 
                                                                  0, 
                                                                  SCHEDULER_CHECK_STATES,
                                                                  SCHEDULER_DEBUG_ACTION_PAUSE,
                                                                  b_is_reboot_test);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Base on the copy request type, initiate the copy request.
     */
    if (validation_event == FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE)
    {
        /* Start the copy request.
         */
        status = fbe_test_sep_background_pause_start_copy(rg_config_p, raid_group_count, position_to_copy,
                                                          copy_type, current_copy_state_p, sep_params_p,
                                                          b_is_reboot_test, dest_array_p);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* If we are just validating that a particular event wait now.
     */
    if (validation_event != FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for Mirror mode", __FUNCTION__);
        event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_MIRROR_MODE);
        status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_copy,
                                                            event_info_p, source_edge_index, dest_edge_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        status = fbe_test_sep_background_pause_wait_for_and_remove_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                   sep_params_p, b_is_reboot_test, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        *current_copy_state_p = FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_MIRROR;
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Mirror mode complete. state: %d ", __FUNCTION__, *current_copy_state_p);
        return status;
    }

    /* Based on the current state take the actions necessary.
     */
    switch(*current_copy_state_p)
    {
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_STARTED:
            status = fbe_test_sep_background_pause_start_proactive_spare_drives(rg_config_p, raid_group_count, position_to_copy,
                                                                                current_copy_state_p,
                                                                                FBE_TEST_SEP_BACKGROUND_COPY_STATE_NOT_APPLICABLE,
                                                                                sep_params_p, b_is_reboot_test);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_START:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_COMPLETE:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_MIRROR:
            status = fbe_test_sep_background_pause_wait_for_swap_in(rg_config_p, raid_group_count, position_to_copy);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        default:
            /* All other cases are unexpected.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s - Unexpected current copy state: %d ",
                       __FUNCTION__, *current_copy_state_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* Return the execution status.
     */
    return status;
}
/************************************************************
 * end fbe_test_sep_background_copy_pause_after_mirror_mode()
 ************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_get_rebuild_checkpoint_to_pause_at()
 *****************************************************************************
 *
 * @brief   Determine the virtual drive rebuild checkpoint (lba) that we should
 *          pause at after the desired `user' space on the associated virtual
 *          drive has been copied.
 *
 * @param   vd_object_id - The virtual drive object id to get the rebuild
 * @param   percent_user_space_copied_before_pause - Percentage of user capacity
 *              on the virtual drive that should be copied before we pause.
 * @param   checkpoint_to_pause_at_p - Address of checkpoint to pause at
 *
 * @return fbe_status_t 
 *
 * @author
 *  11/16/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_pause_get_rebuild_checkpoint_to_pause_at(fbe_object_id_t vd_object_id,
                                                                              fbe_u32_t percent_user_space_copied_before_pause,
                                                                              fbe_lba_t *checkpoint_to_pause_at_p)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_base_config_upstream_object_list_t  upstream_object_list;
    fbe_object_id_t                             rg_object_id;
    fbe_api_raid_group_get_info_t               raid_group_info;
    fbe_u32_t                                   position;
    fbe_api_get_block_edge_info_t               edge_info;
    fbe_lba_t                                   upstream_edge_offset = FBE_LBA_INVALID;
    fbe_lba_t                                   vd_user_capacity = 0;
    fbe_lba_t                                   debug_hook_lba = FBE_LBA_INVALID;

    /* Get the upstream object list (this method only supports (1) upstream
     * object).  
     */
    status = fbe_api_base_config_get_upstream_object_list(vd_object_id, &upstream_object_list);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_TRUE(upstream_object_list.number_of_upstream_objects == 1);
    rg_object_id = upstream_object_list.upstream_object_list[0];
    MUT_ASSERT_INT_NOT_EQUAL(0, rg_object_id);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_OBJECT_ID_INVALID, rg_object_id);

    /* Get the upstream raid group info.
     */
    status = fbe_api_raid_group_get_info(rg_object_id, &raid_group_info, FBE_PACKET_FLAG_NO_ATTRIB);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Locate the downstream virtual drive that we are interested in.
     */
    for (position = 0; position < raid_group_info.width; position++)
    {
        /* Get the edge info for this position
         */
        status = fbe_api_get_block_edge_info(rg_object_id, position, &edge_info, FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* If this is the virtual drive we are interested in get the offset.
         */
        if (edge_info.server_id == vd_object_id)
        {
            upstream_edge_offset = edge_info.offset;
            break;
        }
    }
    MUT_ASSERT_TRUE(upstream_edge_offset != FBE_LBA_INVALID);

    /* Now get the user capacity that is consumed by the virtual drive.
     */
    vd_user_capacity = raid_group_info.capacity / raid_group_info.num_data_disk;

    /* Get the rebuild lba to pause at without the offset.
     */
    debug_hook_lba = ((vd_user_capacity + 99) / 100) * percent_user_space_copied_before_pause;

    /* Now add the offset since the virtual drive will see the lba plus the
     * offset but we want to skip over the unconsumed space.
     */
    debug_hook_lba = debug_hook_lba + upstream_edge_offset;

    /* Display the values we generated.
     */
    mut_printf(MUT_LOG_TEST_STATUS, 
               "fbe_test_sep_background_pause: rg obj: 0x%x vd obj: 0x%x pause percent: %d lba: 0x%llx",
               rg_object_id, vd_object_id, percent_user_space_copied_before_pause, debug_hook_lba);

    /* Update requested value.
     */
    *checkpoint_to_pause_at_p = debug_hook_lba;

    return status;
}
/*************************************************************************
 * end fbe_test_sep_background_pause_get_rebuild_checkpoint_to_pause_at()
 *************************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_set_copy_start_hook()
 *****************************************************************************
 *
 * @brief   After the desired percentage has been copied, `pause' the copy
 *          process.  Or if we the `copy_state_to_pause_at' is some other value
 *          set the proper debug hook.
 *
 * @param   rg_config_p - Pointer raid group information  
 * @param   rg_index - The index of this raid group
 * @param   position_to_copy - raid group position to copy for
 * @param   validation_event - Event to validate (`None' indicates no validation)
 * @param   sep_params_p - Pointer to sep parameters to update
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test.
 *                             FBE_FALSE - This is not a reboot test.
 * @param   dest_array_p - Pointer to array of destination drives for Copy To
 * @param   copy_state_to_pause_at - The specific copy state that we should pause at
 * @param   percent_copied_before_pause - Percentage of raid group capacity that
 *              is copied before we `pause' (add a switch hook).
 *
 * @return fbe_status_t 
 *
 * @author
 *  07/12/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_set_copy_start_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                      fbe_u32_t rg_index,
                                                                      fbe_u32_t position_to_copy,
                                                                      fbe_sep_package_load_params_t *sep_params_p,
                                                                      fbe_bool_t b_is_reboot_test,
                                                                      fbe_test_sep_background_copy_state_t copy_state_to_pause_at,
                                                                      fbe_u32_t percent_copied_before_pause)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t     rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t     vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t    source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t    dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_lba_t           rebuild_checkpoint_to_pause_at = FBE_LBA_INVALID;

    /* Get the vd object id of the position to force into proactive copy
     */
    fbe_api_database_lookup_raid_group_by_number(rg_config_p->raid_group_id, &rg_object_id);
    status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

    /*! @note Get the edge index of the source and destination drive based 
     *        on configuration mode.
     *
     *  @note This assumes that all raid groups have the same source and 
     *        destination index. 
     */
    fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
    MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

    /* Set the proper debug hook based on the requested `pause at' state.
     */
    switch(copy_state_to_pause_at)
    {
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT:  
            /* Determine the rebuild checkpoint to pause at.
             */
            status = fbe_test_sep_background_pause_get_rebuild_checkpoint_to_pause_at(vd_object_id,
                                                                                      percent_copied_before_pause,
                                                                                      &rebuild_checkpoint_to_pause_at);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Set the debug hook
             */
            status = fbe_test_sep_background_pause_set_debug_hook(&sep_params_p->scheduler_hooks[rg_index], rg_index,
                                                                  vd_object_id,
                                                                  SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD,
                                                                  FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY,
                                                                  rebuild_checkpoint_to_pause_at, 
                                                                  0, 
                                                                  SCHEDULER_CHECK_VALS_GT,
                                                                  SCHEDULER_DEBUG_ACTION_PAUSE,
                                                                  b_is_reboot_test);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
            
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_START:
            /* Set the debug hook.  Need to set a global to allow the start to
             * work.
             */
            status = fbe_test_sep_background_pause_set_debug_hook(&sep_params_p->scheduler_hooks[rg_index], rg_index,
                                                                  vd_object_id,
                                                                  SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY,
                                                                  FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START,
                                                                  0, 
                                                                  0, 
                                                                  SCHEDULER_CHECK_STATES,
                                                                  SCHEDULER_DEBUG_ACTION_PAUSE,
                                                                  b_is_reboot_test);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            fbe_test_sep_background_pause_b_is_rebuild_start_hook_set = FBE_TRUE;
            break;
                
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_COMPLETE:   
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_START:  
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_COMPLETE:
        default:
            /* Unsupported `pause at' state.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s rg obj: 0x%x vd obj: 0x%x unsupported pause at state: %d",
                       __FUNCTION__, rg_object_id, vd_object_id, copy_state_to_pause_at);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
    }

    /* Return the execution status.
     */
    return status;
}
/*********************************************************
 * end fbe_test_sep_background_pause_set_copy_start_hook()
 *********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_copy_pause_after_copy_start()
 *****************************************************************************
 *
 * @brief   After the desired percentage has bene copied, `pause' the copy
 *          process.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to copy for
 * @param   copy_type - Type of copy operation requested
 * @param   current_copy_state_p - Address of current copy state to update
 * @param   validation_event - Event to validate (`None' indicates no validation)
 * @param   sep_params_p - Pointer to sep parameters to update
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test.
 *                             FBE_FALSE - This is not a reboot test.
 * @param   dest_array_p - Pointer to array of destination drives for Copy To
 * @param   copy_state_to_pause_at - The specific copy state that we should pause at
 * @param   percent_copied_before_pause - Percentage of raid group capacity that
 *              is copied before we `pause' (add a switch hook).
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/27/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_copy_pause_after_copy_start(fbe_test_rg_configuration_t *rg_config_p,
                                                                 fbe_u32_t raid_group_count,
                                                                 fbe_u32_t position_to_copy,
                                                                 fbe_spare_swap_command_t copy_type,
                                                                 fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                                 fbe_test_sep_background_copy_event_t validation_event,
                                                                 fbe_sep_package_load_params_t *sep_params_p,
                                                                 fbe_bool_t b_is_reboot_test,
                                                                 fbe_test_raid_group_disk_set_t *dest_array_p,
                                                                 fbe_test_sep_background_copy_state_t copy_state_to_pause_at,
                                                                 fbe_u32_t percent_copied_before_pause)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_spare_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_test_sep_background_ops_event_info_t *event_info_p = NULL;

    /* Now set the debug hook as required.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the drive to spare
         */
        drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Get the edge index of the source and destination drive based on configuration mode. */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Only set the debug hook if we are not validating that an event occurred.
         */
        if (validation_event == FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE)
        {
            /* Determine the rebuild checkpoint to pause at.
             */
            status = fbe_test_sep_background_pause_set_copy_start_hook(current_rg_config_p, rg_index, position_to_copy,
                                                                       sep_params_p, b_is_reboot_test,
                                                                       copy_state_to_pause_at, percent_copied_before_pause);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        }

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* If we are just validating that a particular event wait now.
     */
    if (validation_event != FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE)
    {
        /* If we are waiting for the rebuild to start first wait for the 
         * metadata rebuild start event.
         */
        if (validation_event == FBE_TEST_SEP_BACKGROUND_COPY_EVENT_METADATA_REBUILD_START)
        {
            /* Wait for the metadata rebuild start event.
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for virtual drive metadata rebuild start", __FUNCTION__);
            event_info_p = fbe_test_sep_background_ops_get_event_info(FBE_TEST_SEP_BACKGROUND_OPS_EVENT_TYPE_METADATA_REBUILD_START);
            status = fbe_test_sep_background_ops_wait_for_event(rg_config_p, raid_group_count, position_to_copy,
                                                  event_info_p, source_edge_index, dest_edge_index);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            mut_printf(MUT_LOG_TEST_STATUS, "%s: Virtual drive metadata rebuild started. state: %d", __FUNCTION__, *current_copy_state_p);
            fbe_test_sep_background_pause_b_is_rebuild_start_hook_set = FBE_FALSE;
        }

        /* Now wait for the debug hook.
         */
        status = fbe_test_sep_background_pause_wait_for_and_remove_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                   sep_params_p, b_is_reboot_test, FBE_TRUE);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Based on the current state take the actions necessary.
     */
    switch(*current_copy_state_p)
    {
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_STARTED:
            if (copy_type == FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND)
            {
                status = fbe_test_sep_background_pause_start_proactive_spare_drives(rg_config_p, raid_group_count, position_to_copy,
                                                                                    current_copy_state_p,
                                                                                    FBE_TEST_SEP_BACKGROUND_COPY_STATE_NOT_APPLICABLE,
                                                                                    sep_params_p, b_is_reboot_test);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            }
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_START:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_COMPLETE:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_MIRROR:
            status = fbe_test_sep_background_pause_start_copy(rg_config_p, raid_group_count, position_to_copy,
                                                              copy_type, current_copy_state_p, sep_params_p,
                                                              b_is_reboot_test, dest_array_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_START:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_COMPLETE:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_START:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_COMPLETE:
            /* Step 1: Wait for the copy to start.
             */
            status = fbe_test_sep_background_pause_wait_for_copy_start(rg_config_p, raid_group_count, position_to_copy,
                                                                       current_copy_state_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        default:
            /* All other cases are unexpected.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s - Unexpected current copy state: %d ",
                       __FUNCTION__, *current_copy_state_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* Return the execution status.
     */
    return status;
}
/***********************************************************
 * end fbe_test_sep_background_copy_pause_after_copy_start()
 ***********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_copy_pause_after_initiate_copy_complete_job()
 *****************************************************************************
 *
 * @brief   After all the data has been copied `pause' at the desired swap-out
 *          point.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to copy for
 * @param   copy_type - Type of copy operation requested
 * @param   current_copy_state_p - Address of current copy state to update
 * @param   copy_state_to_pause_at - The copy state to `pause' at
 * @param   validation_event - Event to validate (`None' indicates no validation)
 * @param   sep_params_p - Pointer to sep parameters to update
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test.
 *                             FBE_FALSE - This is not a reboot test.
 * @param   dest_array_p - Pointer to array of destination drives for Copy To
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/27/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_copy_pause_after_initiate_copy_complete_job(fbe_test_rg_configuration_t *rg_config_p,
                                                                 fbe_u32_t raid_group_count,
                                                                 fbe_u32_t position_to_copy,
                                                                 fbe_spare_swap_command_t copy_type,
                                                                 fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                                 fbe_test_sep_background_copy_event_t validation_event,
                                                                 fbe_sep_package_load_params_t *sep_params_p,
                                                                 fbe_bool_t b_is_reboot_test,
                                                                 fbe_test_raid_group_disk_set_t *dest_array_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Only set the debug hook if we are not validating that an event occurred.
     */
    if (validation_event == FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE)
    {
        /* Set the swap-out hook
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Set initiate copy complete hook", __FUNCTION__);
        status = fbe_test_sep_background_pause_set_initiate_copy_complete_hook(rg_config_p, raid_group_count, position_to_copy,
                                                            sep_params_p, b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Set initiate copy complete hook - complete.", __FUNCTION__);
    }
    else
    {
        /* Else if we are just validating that a particular event wait now.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for initiate copy complete hook", __FUNCTION__);
        status = fbe_test_sep_background_pause_wait_and_clear_initiate_copy_complete_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                 sep_params_p, b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for initiate copy complete hook - complete.", __FUNCTION__);
        return status;
    }

    /* Based on the current state take the actions necessary.
     */
    switch(*current_copy_state_p)
    {
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_STARTED:
            status = fbe_test_sep_background_pause_start_proactive_spare_drives(rg_config_p, raid_group_count, position_to_copy,
                                                                                current_copy_state_p,
                                                                                FBE_TEST_SEP_BACKGROUND_COPY_STATE_NOT_APPLICABLE,
                                                                                sep_params_p, b_is_reboot_test);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_START:

            status = fbe_test_sep_background_pause_start_copy(rg_config_p, raid_group_count, position_to_copy,
                                                              copy_type, current_copy_state_p, sep_params_p,
                                                              b_is_reboot_test, dest_array_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_COMPLETE:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_MIRROR:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_START:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_COMPLETE:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_START:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT:
            /* Step 1: Wait for the copy to start.
             */
            status = fbe_test_sep_background_pause_copy_started(rg_config_p, raid_group_count, position_to_copy,
                                                                current_copy_state_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_COMPLETE:
            /* Step 1: Wait for the copy to complete.
             */
            status = fbe_test_sep_background_pause_wait_for_copy_complete(rg_config_p, raid_group_count, position_to_copy,
                                                                          current_copy_state_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_INITIATE_COPY_COMPLETE_JOB:
            /* Step 1: Wait for initiate copy complete (abort) job to start
             */
            status = fbe_test_sep_background_pause_wait_for_initiate_copy_complete_job(rg_config_p, raid_group_count, position_to_copy,
                                                                     current_copy_state_p, sep_params_p, b_is_reboot_test);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        default:
            /* All other cases are unexpected.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s - Unexpected current copy state: %d ",
                       __FUNCTION__, *current_copy_state_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* Return the execution status.
     */
    return status;
}
/***********************************************************
 * end fbe_test_sep_background_copy_pause_after_initiate_copy_complete_job()
 ***********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_copy_pause_after_swap_out_complete()
 *****************************************************************************
 *
 * @brief   After all the data has been copied `pause' at the desired swap-out
 *          point.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to copy for
 * @param   copy_type - Type of copy operation requested
 * @param   current_copy_state_p - Address of current copy state to update
 * @param   copy_state_to_pause_at - The copy state to `pause' at
 * @param   validation_event - Event to validate (`None' indicates no validation)
 * @param   sep_params_p - Pointer to sep parameters to update
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test.
 *                             FBE_FALSE - This is not a reboot test.
 * @param   dest_array_p - Pointer to array of destination drives for Copy To
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/27/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_copy_pause_after_swap_out_complete(fbe_test_rg_configuration_t *rg_config_p,
                                                                 fbe_u32_t raid_group_count,
                                                                 fbe_u32_t position_to_copy,
                                                                 fbe_spare_swap_command_t copy_type,
                                                                 fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                                 fbe_test_sep_background_copy_event_t validation_event,
                                                                 fbe_sep_package_load_params_t *sep_params_p,
                                                                 fbe_bool_t b_is_reboot_test,
                                                                 fbe_test_raid_group_disk_set_t *dest_array_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Only set the debug hook if we are not validating that an event occurred.
     */
    if (validation_event == FBE_TEST_SEP_BACKGROUND_COPY_EVENT_NONE)
    {
        /* Set the swap-out hook
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Set swap-out complete hook", __FUNCTION__);
        status = fbe_test_sep_background_pause_set_swap_out_complete_hook(rg_config_p, raid_group_count, position_to_copy,
                                                            sep_params_p, b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Set swap-out complete hook - complete.", __FUNCTION__);
    }
    else
    {
        /* Else if we are just validating that a particular event wait now.
         */
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for swap-out complete hook", __FUNCTION__);
        status = fbe_test_sep_background_pause_wait_and_clear_swap_out_complete_hook(rg_config_p, raid_group_count, position_to_copy,
                                                                 sep_params_p, b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        mut_printf(MUT_LOG_TEST_STATUS, "%s: Wait for swap-out complete hook - complete.", __FUNCTION__);
        return status;
    }

    /* Based on the current state take the actions necessary.
     */
    switch(*current_copy_state_p)
    {
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_STARTED:
            status = fbe_test_sep_background_pause_start_proactive_spare_drives(rg_config_p, raid_group_count, position_to_copy,
                                                                                current_copy_state_p,
                                                                                FBE_TEST_SEP_BACKGROUND_COPY_STATE_NOT_APPLICABLE,
                                                                                sep_params_p, b_is_reboot_test);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_START:

            status = fbe_test_sep_background_pause_start_copy(rg_config_p, raid_group_count, position_to_copy,
                                                              copy_type, current_copy_state_p, sep_params_p,
                                                              b_is_reboot_test, dest_array_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_COMPLETE:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_MIRROR:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_START:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_COMPLETE:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_START:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT:
            /* Step 1: Wait for the copy to start.
             */
            status = fbe_test_sep_background_pause_copy_started(rg_config_p, raid_group_count, position_to_copy,
                                                                current_copy_state_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_COMPLETE:
            /* Step 1: Wait for the copy to complete.
             */
            status = fbe_test_sep_background_pause_wait_for_copy_complete(rg_config_p, raid_group_count, position_to_copy,
                                                                          current_copy_state_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_INITIATE_COPY_COMPLETE_JOB:
            /* Step 1: Wait for initiate copy complete (abort) job to start
             */
            status = fbe_test_sep_background_pause_wait_for_initiate_copy_complete_job(rg_config_p, raid_group_count, position_to_copy,
                                                                     current_copy_state_p, sep_params_p, b_is_reboot_test);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_PASS_THRU:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_SWAP_OUT_START:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_SWAP_OUT_COMPLETE:
            /* Step 1: Wait for swap-out complete start.
             */
            status = fbe_test_sep_background_pause_wait_for_swap_out_complete_start(rg_config_p, raid_group_count, position_to_copy,
                                                                     current_copy_state_p);
            break;

        default:
            /* All other cases are unexpected.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s - Unexpected current copy state: %d ",
                       __FUNCTION__, *current_copy_state_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* Return the execution status.
     */
    return status;
}
/******************************************************************
 * end fbe_test_sep_background_copy_pause_after_swap_out_complete()
 ******************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_copy_pause_copy_cleanup()
 *****************************************************************************
 *
 * @brief   Perform any cleanup of the copy operation.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to copy for
 * @param   copy_type - Type of copy operation requested
 * @param   current_copy_state_p - Address of current copy state to update
 * @param   copy_state_to_pause_at - The copy state to `pause' at
 * @param   validation_event - Event to validate (`None' indicates no validation)
 * @param   sep_params_p - Pointer to sep parameters to update
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test.
 *                             FBE_FALSE - This is not a reboot test.
 * @param   dest_array_p - Pointer to array of destination drives for Copy To
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/27/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_copy_pause_copy_cleanup(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position_to_copy,
                                                               fbe_spare_swap_command_t copy_type,
                                                               fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                               fbe_test_sep_background_copy_event_t validation_event,
                                                               fbe_sep_package_load_params_t *sep_params_p,
                                                               fbe_bool_t b_is_reboot_test,
                                                               fbe_test_raid_group_disk_set_t *dest_array_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       hook_index;

    /* Based on the current state take the actions necessary.
     */
    switch(*current_copy_state_p)
    {
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_REQUESTED:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_STARTED:
            /* Based on the copy type start the copy operation.
             */
            switch(copy_type)
            {
                case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
                    /* Start proactive copy.*/
                    status = fbe_test_sep_background_pause_start_proactive_spare_drives(rg_config_p, raid_group_count, position_to_copy,
                                                                                current_copy_state_p,
                                                                                FBE_TEST_SEP_BACKGROUND_COPY_STATE_NOT_APPLICABLE,
                                                                                sep_params_p, b_is_reboot_test);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    break;

                case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
                case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
                    /* Start the user copy request. */
                    status = fbe_test_sep_background_pause_start_copy(rg_config_p, raid_group_count, position_to_copy,
                                                              copy_type, current_copy_state_p, sep_params_p,
                                                              b_is_reboot_test, dest_array_p);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    break;

                default:
                    /* Invalid copy type.*/
                    status = FBE_STATUS_GENERIC_FAILURE;
                    mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s - Unexpected current copy type: %d ",
                       __FUNCTION__, copy_type);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    return status;
            }
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_START:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_COMPLETE:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_MIRROR:
            /* Step 1: Wait for the copy to start.
             */
            status = fbe_test_sep_background_pause_copy_started(rg_config_p, raid_group_count, position_to_copy,
                                                                current_copy_state_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_START:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_COMPLETE:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_START:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_COMPLETE:
            /* Step 1: Wait for the copy to complete.
             */
            status = fbe_test_sep_background_pause_copy_complete(rg_config_p, raid_group_count, position_to_copy,
                                                                 current_copy_state_p, sep_params_p, b_is_reboot_test);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            // Fall-thru
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_INITIATE_COPY_COMPLETE_JOB:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_PASS_THRU:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_SWAP_OUT_START:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_SWAP_OUT_COMPLETE:
            /* Step 1: Perform copy cleanup.
             */
            status = fbe_test_sep_background_pause_copy_cleanup(rg_config_p, raid_group_count, position_to_copy,
                                                                copy_type, current_copy_state_p, sep_params_p, b_is_reboot_test);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        default:
            /* All other cases are unexpected.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "%s - Unexpected current copy state: %d ",
                       __FUNCTION__, *current_copy_state_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* Unless `reboot test' is set clear all the debug hooks.
     */
    if (b_is_reboot_test == FBE_FALSE)
    {
        mut_printf(MUT_LOG_TEST_STATUS, "%s - Clear all debug hooks start", __FUNCTION__);

        /* First clear the SP debug hooks (only on the local SP)
         */
        fbe_api_scheduler_clear_all_debug_hooks(&sep_params_p->scheduler_hooks[0]);

        /* Now clear the local copy of the hooks.
         */
        for (hook_index = 0; hook_index < MAX_SCHEDULER_DEBUG_HOOKS; hook_index++)
        {
            /* Only zero it if it is valid.
             */
            if ((sep_params_p->scheduler_hooks[hook_index].object_id != 0)                                  &&
                (sep_params_p->scheduler_hooks[hook_index].object_id != FBE_OBJECT_ID_INVALID)              &&
                (sep_params_p->scheduler_hooks[hook_index].monitor_state > SCHEDULER_MONITOR_STATE_INVALID) &&
                (sep_params_p->scheduler_hooks[hook_index].monitor_state < SCHEDULER_MONITOR_STATE_LAST)       )
            {
                fbe_zero_memory(&sep_params_p->scheduler_hooks[hook_index], sizeof(fbe_scheduler_debug_hook_t));
            }
        }

        mut_printf(MUT_LOG_TEST_STATUS, "%s - Clear all debug hooks - complete", __FUNCTION__);
    }

    /* Return the execution status.
     */
    return status;
}
/***********************************************************
 * end fbe_test_sep_background_copy_pause_copy_cleanup()
 ***********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_validate_copy_request()
 *****************************************************************************
 *
 * @brief   Validate the parameters based on the type of copy requested.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to copy
 * @param   copy_type - Type of copy request
 * @param   current_copy_state_p - Address of current copy state to update
 * @param   copy_state_to_pause_at - The copy state to `pause' at
 * @param   validation_event - Event to validate (`None' indicates no validation)
 * @param   sep_params_p - Pointer to sep parameters to update
 * @param   percent_copied_before_pause - Percentage of raid group capacity that
 *              is copied before we `pause' (add a switch hook).
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test.
 *                             FBE_FALSE - This is not a reboot test.
 * @param   dest_array_p - Pointer to array of destination drives for Copy To.
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/22/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_pause_validate_copy_request(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position_to_copy,
                                                               fbe_spare_swap_command_t copy_type,
                                                               fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                               fbe_test_sep_background_copy_state_t copy_state_to_pause_at,
                                                               fbe_test_sep_background_copy_event_t validation_event,
                                                               fbe_sep_package_load_params_t *sep_params_p,
                                                               fbe_u32_t percent_copied_before_pause,
                                                               fbe_bool_t b_is_reboot_test,
                                                               fbe_test_raid_group_disk_set_t *dest_array_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Currently unreferenced parameters.
     */
    FBE_UNREFERENCED_PARAMETER(rg_config_p);
    FBE_UNREFERENCED_PARAMETER(position_to_copy);
    FBE_UNREFERENCED_PARAMETER(sep_params_p);
    FBE_UNREFERENCED_PARAMETER(percent_copied_before_pause);

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Copy test not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Validate that there aren't too many raid groups
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Validate rebuild percentage.
     */
    if (((fbe_s32_t)percent_copied_before_pause < 0) ||
        (percent_copied_before_pause > 100)             )
    {
        /* Invalid rebuild percentage
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid rebuild percentage: %d", 
                   __FUNCTION__, percent_copied_before_pause);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Based on the copy request type validate the request.
     */
    switch(copy_type)
    {
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
            /* For both System Initiated and User Initiated Proactive Copy
             * the destination is selected by the system.
             */
            if (dest_array_p != NULL)
            {
                /* Cannot specify either a source or destination for proactive copy.
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "%s Cannot specify dest: (%d_%d_%d) pvd for copy_type: %d", 
                           __FUNCTION__, dest_array_p->bus, dest_array_p->enclosure, dest_array_p->slot, copy_type);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
            }
            break;

        case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
            /* For both System Initiated and User Initiated Proactive Copy
             * the destination is selected by the system.
             */
            if (dest_array_p != NULL)
            {
                /* Cannot specify either a source or destination for proactive copy.
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "%s Cannot specify dest: (%d_%d_%d) pvd for copy_type: %d", 
                           __FUNCTION__, dest_array_p->bus, dest_array_p->enclosure, dest_array_p->slot, copy_type);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
            }

            /* Unexpected current copy states.
             */
            switch (*current_copy_state_p)
            {
                case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL:
                    /* This isn't support for user copy.
                     */
                    status = FBE_STATUS_GENERIC_FAILURE;
                    mut_printf(MUT_LOG_TEST_STATUS, "%s Unexpected current state: %d for copy_type: %d", 
                               __FUNCTION__, *current_copy_state_p, copy_type);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    return status;
                    
                default:
                    break;
            }

            /* Validate the copy state to pause at is supported.
             */
            switch (copy_state_to_pause_at)
            {
                case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL:
                    /* This isn't support for user copy.
                     */
                    status = FBE_STATUS_GENERIC_FAILURE;
                    mut_printf(MUT_LOG_TEST_STATUS, "%s Unexpected state to pause at: %d for copy_type: %d", 
                               __FUNCTION__, *current_copy_state_p, copy_type);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    return status;
                    
                default:
                    break;
            }

            /* Validate that the expected state is ok for this copy type.
             */
            switch(validation_event)
            {
                case FBE_TEST_SEP_BACKGROUND_COPY_EVENT_SOURCE_MARKED_EOL:
                    /* This isn't support for user copy.
                     */
                    status = FBE_STATUS_GENERIC_FAILURE;
                    mut_printf(MUT_LOG_TEST_STATUS, "%s Unexpected validation event: %d for copy_type: %d", 
                               __FUNCTION__, validation_event, copy_type);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    return status;

                default:
                    break;
            }
            break;

        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            /* The destination array must be valid.
             */
            if (dest_array_p == NULL)
            {
                /* Must specify destination.
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "%s Must specify destination for copy_type: %d ", 
                           __FUNCTION__, copy_type);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return status;
            }

            /* Unexpected current copy states.
             */
            switch (*current_copy_state_p)
            {
                case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL:
                    /* This isn't support for user copy.
                     */
                    status = FBE_STATUS_GENERIC_FAILURE;
                    mut_printf(MUT_LOG_TEST_STATUS, "%s Unexpected current state: %d for copy_type: %d", 
                               __FUNCTION__, *current_copy_state_p, copy_type);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    return status;
            }

            /* Validate the copy state to pause at is supported.
             */
            switch (copy_state_to_pause_at)
            {
                case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL:
                    /* This isn't support for user copy.
                     */
                    status = FBE_STATUS_GENERIC_FAILURE;
                    mut_printf(MUT_LOG_TEST_STATUS, "%s Unexpected state to pause at: %d for copy_type: %d", 
                               __FUNCTION__, *current_copy_state_p, copy_type);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    return status;
                    
                default:
                    break;
            }

            /* Validate that the expected state is ok for this copy type.
             */
            switch(validation_event)
            {
                case FBE_TEST_SEP_BACKGROUND_COPY_EVENT_SOURCE_MARKED_EOL:
                    /* This isn't support for user copy.
                     */
                    status = FBE_STATUS_GENERIC_FAILURE;
                    mut_printf(MUT_LOG_TEST_STATUS, "%s Unexpected validation event: %d for copy_type: %d", 
                               __FUNCTION__, validation_event, copy_type);
                    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                    return status;

                default:
                    break;
            }
            break;

        default:
            /* Unsupported copy request.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s Unsupported copy_type: %d ", 
                       __FUNCTION__, copy_type);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            return status;
    }

    return status;
}
/***********************************************************
 * end fbe_test_sep_background_pause_validate_copy_request()
 ***********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_run_copy_operation_with_pause()
 *****************************************************************************
 * @brief
 *  Forces drives into proactive sparing, one per raid group, then `pauses'
 *  once the copy operation reaches the percentage of raid group capacity
 *  requested.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - raid group position to copy
 * @param   copy_type - Type of copy request
 * @param   current_copy_state_p - Address of current copy state to update
 * @param   copy_state_to_pause_at - The copy state to `pause' at
 * @param   validation_event - Event to validate (`None' indicates no validation)
 * @param   sep_params_p - Pointer to sep parameters to update
 * @param   percent_copied_before_pause - Percentage of raid group capacity that
 *              is copied before we `pause' (add a switch hook).
 * @param   b_is_reboot_test - FBE_TRUE - This is a reboot test.
 *                             FBE_FALSE - This is not a reboot test.
 * @param   dest_array_p - Pointer to array of destination drives for Copy To
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/22/2012  Ron Proulx  - Updated.
 *
 *****************************************************************************/
static fbe_status_t fbe_test_sep_background_run_copy_operation_with_pause(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position_to_copy,
                                                               fbe_spare_swap_command_t copy_type,
                                                               fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                               fbe_test_sep_background_copy_state_t copy_state_to_pause_at,
                                                               fbe_test_sep_background_copy_event_t validation_event,
                                                               fbe_sep_package_load_params_t *sep_params_p,
                                                               fbe_u32_t percent_copied_before_pause,
                                                               fbe_bool_t b_is_reboot_test,
                                                               fbe_test_raid_group_disk_set_t *dest_array_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Validate the input parameters.
     */
    status = fbe_test_sep_background_pause_validate_copy_request(rg_config_p, raid_group_count, position_to_copy,
                                                                 copy_type, current_copy_state_p, copy_state_to_pause_at,
                                                                 validation_event, sep_params_p, percent_copied_before_pause,
                                                                 b_is_reboot_test, dest_array_p);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Set a global since it must be the same for all raid groups.
     */
    fbe_test_sep_background_pause_wait_for_percent_rebuilt_value = percent_copied_before_pause;

    /* Due to reboots etc, extend the spare operation timeout since we wait
     * for debug hooks across reboots.
     */
    if (copy_state_to_pause_at != FBE_TEST_SEP_BACKGROUND_COPY_STATE_COPY_OPERATION_COMPLETE)
    {
        status = fbe_test_sep_util_disable_spare_library_confirmation();
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    }

    /* Based on the desired state to pause at invoke the appropriate method.
     */
    switch(copy_state_to_pause_at)
    {
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_MARKED_EOL:
            status = fbe_test_sep_background_copy_pause_after_eol(rg_config_p, raid_group_count, position_to_copy,
                                                                  current_copy_state_p,
                                                                  validation_event,
                                                                  sep_params_p,
                                                                  b_is_reboot_test);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;    
                  
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_OPERATION_STARTED:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_START:   
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESTINATION_SWAP_IN_COMPLETE:
            status = fbe_test_sep_background_copy_pause_after_swap_in(rg_config_p, raid_group_count, position_to_copy,
                                                                      copy_type, current_copy_state_p, validation_event, 
                                                                      sep_params_p, b_is_reboot_test, dest_array_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_MIRROR:
            status = fbe_test_sep_background_copy_pause_after_mirror_mode(rg_config_p, raid_group_count, position_to_copy,
                                                                      copy_type, current_copy_state_p, validation_event, 
                                                                      sep_params_p, b_is_reboot_test, dest_array_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_START:      
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_METADATA_REBUILD_COMPLETE:   
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_START:  
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_DESIRED_PERCENTAGE_REBUILT:  
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_USER_REBUILD_COMPLETE: 
            status = fbe_test_sep_background_copy_pause_after_copy_start(rg_config_p, raid_group_count, position_to_copy,
                                                                         copy_type, current_copy_state_p, validation_event, 
                                                                         sep_params_p, b_is_reboot_test, dest_array_p,
                                                                         copy_state_to_pause_at, percent_copied_before_pause);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_INITIATE_COPY_COMPLETE_JOB:
            status = fbe_test_sep_background_copy_pause_after_initiate_copy_complete_job(rg_config_p, raid_group_count, position_to_copy,
                                                                       copy_type, current_copy_state_p, validation_event, 
                                                                       sep_params_p, b_is_reboot_test, dest_array_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;  

        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_MODE_SET_TO_PASS_THRU:
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_SWAP_OUT_START:       
        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_SOURCE_SWAP_OUT_COMPLETE:    
            status = fbe_test_sep_background_copy_pause_after_swap_out_complete(rg_config_p, raid_group_count, position_to_copy,
                                                                       copy_type, current_copy_state_p, validation_event, 
                                                                       sep_params_p, b_is_reboot_test, dest_array_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break; 

        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_COPY_OPERATION_COMPLETE:
            status = fbe_test_sep_background_copy_pause_copy_cleanup(rg_config_p, raid_group_count, position_to_copy,
                                                                     copy_type, current_copy_state_p, validation_event, 
                                                                     sep_params_p, b_is_reboot_test, dest_array_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* Restore the spare operation confirmation
             */
            status = fbe_test_sep_util_enable_spare_library_confirmation();
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            break; 

        case FBE_TEST_SEP_BACKGROUND_COPY_STATE_INVALID:
        default:
            break;
    }


    /* Return the execution status.
     */
    return status;
}
/*****************************************************************
 * end fbe_test_sep_background_run_copy_operation_with_pause()
 *****************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_copy_operation_with_pause()
 ***************************************************************************** 
 * 
 * @brief   Start a copy operation (of the type requested) on the raid group
 *          array specified then `pause' the copy operation when the percentage
 *          copied is greater than or equal to the value specified.
 * 
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array
 * @param   position_to_copy - The position in the raid group to copy from
 * @param   copy_type - Type of copy command to initiate
 * @param   current_copy_state_p - Address of current copy state to update
 * @param   copy_state_to_pause_at - The copy state to `pause' at
 * @param   validation_event - Event to validate (`None' indicates no validation)
 * @param   sep_params_p - Pointer to sep parameters to update
 * @param   percent_copied_before_pause - Percent of raid group copied before pause
 * @param   dest_array_p - Array of destination drives to copy to
 *
 * @return fbe_status_t 
 *
 * @author
 *  06/22/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_copy_operation_with_pause(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position_to_copy,
                                                               fbe_spare_swap_command_t copy_type,
                                                               fbe_test_sep_background_copy_state_t *current_copy_state_p,
                                                               fbe_test_sep_background_copy_state_t copy_state_to_pause_at,
                                                               fbe_test_sep_background_copy_event_t validation_event,
                                                               fbe_sep_package_load_params_t *sep_params_p,
                                                               fbe_u32_t percent_copied_before_pause,
                                                               fbe_bool_t b_is_reboot_test,
                                                               fbe_test_raid_group_disk_set_t *dest_array_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Based on the requested copy type, initiate the copy request.
     */
    switch(copy_type)
    {
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
        case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            /* All copy operations use hte same method.*/
            status = fbe_test_sep_background_run_copy_operation_with_pause(rg_config_p, 
                                                                           raid_group_count,
                                                                           position_to_copy,
                                                                           copy_type,
                                                                           current_copy_state_p,
                                                                           copy_state_to_pause_at,
                                                                           validation_event,
                                                                           sep_params_p,
                                                                           percent_copied_before_pause,
                                                                           b_is_reboot_test,
                                                                           dest_array_p);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;

        case FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND:
            /*! @todo Currently these copy operations are not supported. 
             */
            mut_printf(MUT_LOG_TEST_STATUS, "%s TO-DO: copy type: %d not supported !", 
                       __FUNCTION__, copy_type);
            break;

        default:
            /* Invalid copy_type specified.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid copy type: %d specified.", 
                       __FUNCTION__, copy_type);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            break;
    }

    /* Return the execution status.
     */
    return status;
}
/*********************************************************
 * end fbe_test_sep_background_copy_operation_with_pause()
 *********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_set_copy_debug_hook()
 *****************************************************************************
 *
 * @brief   Set the requested debug hooks for the associated virtual drive
 *          objects.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - The position in the raid group to copy from
 * @param   hook_array_p - Pointer to array of hooks to use
 *
 * @return  fbe_status_t 
 *
 * @author
 *  07/11/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_pause_set_copy_debug_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position_to_copy,
                                                               fbe_sep_package_load_params_t *hook_array_p,
                                                               fbe_u32_t monitor_state,
                                                               fbe_u32_t monitor_substate,
                                                               fbe_u64_t val1,
                                                               fbe_u64_t val2,
                                                               fbe_u32_t check_type,
                                                               fbe_u32_t action,
                                                               fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_raid_group_disk_set_t *drive_to_copy_p = NULL;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Forcing proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Validate the destination array is not NULL
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Step 1: Locate the associated virtual drive and set the debug hook.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* We need to add the drive to the `needing spare' position primarily for
         * debug purposes.
         */
        drive_to_copy_p = &current_rg_config_p->rg_disk_set[position_to_copy];

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /*! @note Get the edge index of the source and destination drive based 
         *        on configuration mode.
         *
         *  @note This assumes that all raid groups have the same source and 
         *        destination index. 
         */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* Set the requested debug hook.
         */
        status = fbe_test_sep_background_pause_set_debug_hook(&hook_array_p->scheduler_hooks[rg_index], rg_index,
                                                              vd_object_id,
                                                              monitor_state,
                                                              monitor_substate,
                                                              val1, val2, 
                                                              check_type,
                                                              action,
                                                              b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);    

        /* Display some information
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Set pause hook on raid: 0x%x pos: %d vd obj: 0x%x(%d_%d_%d) source: %d dest: %d(n/a)",
                   rg_object_id, position_to_copy, vd_object_id,
                   drive_to_copy_p->bus, drive_to_copy_p->enclosure, drive_to_copy_p->slot,
                   source_edge_index, dest_edge_index);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Return the execution status.
     */
    return status;
}
/*********************************************************
 * end fbe_test_sep_background_pause_set_copy_debug_hook()
 *********************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_remove_copy_debug_hook()
 *****************************************************************************
 *
 * @brief   Remove the hook form the active SP for all the active objects                         
 *
 * @param   rg_config_p - Current raid group config
 * @param   raid_group_count - The index of the raid group arrays to set debug hook for
 * @param   position_to_copy - Raid group position that is being spared
 * @param   b_is_reboot_test - FBE_TRUE this is a reboot test
 *                             FBE_FALSE this is not a reboot test
 *
 * @return  fbe_status_t 
 *
 * @author
 *  06/02/2014  - Ron Proulx    - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_pause_remove_copy_debug_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                                  fbe_u32_t raid_group_count,
                                                                  fbe_u32_t position_to_copy,
                                                                  fbe_sep_package_load_params_t *sep_params_p,
                                                                  fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_u32_t                       rg_index;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_bool_t                      b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();

    /*! @note For dualsp and reboot don't remove any hooks.
     */
    if ((b_is_dualsp_mode == FBE_TRUE) &&
        (b_is_reboot_test == FBE_TRUE)    )
    {
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Both dualsp: %d and reboot: %d set - skip", 
                   __FUNCTION__, b_is_dualsp_mode, b_is_reboot_test);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Remove the hook for each virtual drive
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Remove the passed copy of the debug hook.
         */
        MUT_ASSERT_TRUE(rg_index < MAX_SCHEDULER_DEBUG_HOOKS);
        status = fbe_test_sep_background_pause_remove_debug_hook(&sep_params_p->scheduler_hooks[rg_index], rg_index,
                                                                 b_is_reboot_test);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Flag the setting the private rebuild hook
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: Remove hook raid obj: 0x%x pos: %d vd obj: 0x%x",
                   rg_object_id, position_to_copy, vd_object_id);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Return the status
     */
    return status;
}
/***********************************************************************
 * end fbe_test_sep_background_pause_remove_copy_debug_hook()
 ***********************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_check_remove_copy_debug_hook()
 *****************************************************************************
 *
 * @brief   Validate that the requested debug hooks for the associated virtual
 *          drive objects have been hit.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - The position in the raid group to copy from
 * @param   hook_array_p - Pointer to array of hooks to use
 * @param   b_is_reboot_test - FBE_TRUE - Need to save hooks across reboot
 *
 * @return  fbe_status_t 
 *
 * @author
 *  07/11/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_pause_check_remove_copy_debug_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                               fbe_u32_t raid_group_count,
                                                               fbe_u32_t position_to_copy,
                                                               fbe_sep_package_load_params_t *hook_array_p,
                                                               fbe_bool_t b_is_reboot_test)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Forcing proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Validate the destination array is not NULL
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Step 1: Wait and validate debug hook
     */
    status = fbe_test_sep_background_pause_wait_for_and_remove_debug_hook(rg_config_p, raid_group_count, position_to_copy,
                                                               hook_array_p, b_is_reboot_test,
                                                               FBE_FALSE /* Don't wait for the vd to become ready */);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Return the execution status.
     */
    return status;
}
/******************************************************************
 * end fbe_test_sep_background_pause_check_remove_copy_debug_hook()
 ******************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_glitch_drives()
 *****************************************************************************
 *
 * @brief   `Glitch' either the source and/or destination drive by either
 *          setting or clearing EOL.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - The position in the raid group to copy from
 * @param   b_glitch_source_drive - FBE_TRUE - `glitch' the source drive
 * @param   b_glitch_dest_drive - FBE_TRUE - `glitch' the destination drive
 *
 * @return  fbe_status_t 
 *
 * @author
 *  10/11/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_pause_glitch_drives(fbe_test_rg_configuration_t *rg_config_p,
                                                         fbe_u32_t raid_group_count,
                                                         fbe_u32_t position_to_copy,
                                                         fbe_bool_t b_glitch_source_drive,
                                                         fbe_bool_t b_glitch_dest_drive)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_u32_t                       rg_index;
    fbe_test_rg_configuration_t    *current_rg_config_p = rg_config_p;
    fbe_object_id_t                 rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_edge_index_t                source_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_edge_index_t                dest_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_api_get_block_edge_info_t   edge_info;
    fbe_object_id_t                 source_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                 dest_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_api_provision_drive_info_t  provision_drive_info;
    fbe_test_raid_group_disk_set_t *drive_to_spare_p = NULL; 

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Forcing proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Validate the destination array is not NULL
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Validate sparing position.
     */
    if ((position_to_copy == FBE_TEST_SEP_BACKGROUND_PAUSE_INVALID_DISK_POSITION) ||
        (position_to_copy >= current_rg_config_p->width)                             )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid copy position: %d",
                   __FUNCTION__, position_to_copy);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /*! @todo Currently `glitching' the destination drives is not supported.
     */
    if (b_glitch_dest_drive == FBE_TRUE)
    {
        status = FBE_STATUS_NOT_INITIALIZED;
        mut_printf(MUT_LOG_TEST_STATUS, "%s TODO: `glitching' destination drives not supported.",
                   __FUNCTION__);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* For all raid groups simply check */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        /* Skip raid groups that are not enabled.
         */
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }

        /* Get the vd object id of the position to force into proactive copy
         */
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /*! @note Get the edge index of the source and destination drive based 
         *        on configuration mode.
         *
         *  @note This assumes that all raid groups have the same source and 
         *        destination index. 
         */
        fbe_test_sep_background_ops_get_source_destination_edge_index(vd_object_id, &source_edge_index, &dest_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, source_edge_index);
        MUT_ASSERT_INT_NOT_EQUAL(FBE_EDGE_INDEX_INVALID, dest_edge_index);

        /* If we need to `glitch' the source do that now.
         */
        if (b_glitch_source_drive == FBE_TRUE)
        {
            /* Get the source pvd information.
             */
            status = fbe_api_get_block_edge_info(vd_object_id, source_edge_index, &edge_info, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            source_pvd_object_id = edge_info.server_id;

            /* Get the pvd info to determine which way we need to `toggle' EOL.
             */
            status = fbe_api_provision_drive_get_info(source_pvd_object_id, &provision_drive_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            if (provision_drive_info.end_of_life_state == FBE_TRUE)
            {
                /* We need to clear EOL.
                 */
                status = fbe_api_provision_drive_clear_eol_state(source_pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
                /* Display some information.                 
                 */                                          
                mut_printf(MUT_LOG_TEST_STATUS, "%s vd obj: 0x%x source pvd obj: 0x%x cleared EOL", 
                           __FUNCTION__, vd_object_id, source_pvd_object_id);
            }
            else
            {
                /* Else we want to `glitch' the provision drive.
                 */
                status = fbe_test_sep_util_glitch_drive(current_rg_config_p, position_to_copy);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                /* Display some information.                 
                 */                                          
                mut_printf(MUT_LOG_TEST_STATUS, "%s vd obj: 0x%x source pvd obj: 0x%x glitched drive", 
                           __FUNCTION__, vd_object_id, source_pvd_object_id);
            }

        } /* end if `glitch' source drive */

        /* If we need to `glitch' the destination do that now.
         */
        if (b_glitch_dest_drive == FBE_TRUE)
        {
            /* Get the destination pvd information.
             */
            status = fbe_api_get_block_edge_info(vd_object_id, dest_edge_index, &edge_info, FBE_PACKAGE_ID_SEP_0);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            dest_pvd_object_id = edge_info.server_id;

            /* Get the pvd info to determine which way we need to `toggle' EOL.
             */
            status = fbe_api_provision_drive_get_info(dest_pvd_object_id, &provision_drive_info);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            if (provision_drive_info.end_of_life_state == FBE_TRUE)
            {
                /* We need to clear EOL.
                 */
                status = fbe_api_provision_drive_clear_eol_state(dest_pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
                /* Display some information.                 
                 */                                          
                mut_printf(MUT_LOG_TEST_STATUS, "%s vd obj: 0x%x dest pvd obj: 0x%x cleared EOL", 
                           __FUNCTION__, vd_object_id, dest_pvd_object_id);
            }
            else
            {
                /* Else set EOL.
                 */
                status = fbe_api_provision_drive_set_eol_state(dest_pvd_object_id, FBE_PACKET_FLAG_NO_ATTRIB);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
    
                /* Display some information.                 
                 */                                          
                mut_printf(MUT_LOG_TEST_STATUS, "%s vd obj: 0x%x dest pvd obj: 0x%x set EOL", 
                           __FUNCTION__, vd_object_id, dest_pvd_object_id);
            }

        } /* end if `glitch' destination drive */

        /* Goto next raid group
         */                
        current_rg_config_p++;

    } /* end for all raid groups */

    return status;
}
/******************************************************************
 * end fbe_test_sep_background_pause_glitch_drives()
 ******************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_wait_for_hook()
 *****************************************************************************
 *
 * @brief   Wait until the associated virtual drives hit the previously
 *          configured rebuild hook.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - The position in the raid group to copy from
 * @param   sep_params_p - Pointer to array of hooks to use
 * @param   b_is_reboot_test - FBE_TRUE - Need to save hooks across reboot
 *
 * @return  fbe_status_t 
 *
 * @author
 *  10/11/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_pause_wait_for_hook(fbe_test_rg_configuration_t *rg_config_p,
                                                         fbe_u32_t raid_group_count,
                                                         fbe_u32_t position_to_copy,
                                                         fbe_sep_package_load_params_t *sep_params_p,
                                                         fbe_bool_t b_is_reboot_test)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               rg_index;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_bool_t                              b_is_dualsp_mode = FBE_FALSE;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   debug_hook_sp;
    fbe_object_id_t                         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                               retry_count;
    fbe_u32_t                               max_retries = FBE_TEST_SEP_BACKGROUND_PAUSE_MAX_SHORT_RETRIES;
    fbe_bool_t                              raid_groups_complete[FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT];
    fbe_u32_t                               num_raid_groups_complete = 0;
    fbe_test_raid_group_disk_set_t         *drive_to_spare_p = NULL; 
    fbe_bool_t                              b_is_debug_hook_reached = FBE_FALSE;
    fbe_u32_t                               rebuild_hook_index = 0;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Forcing proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Validate the destination array is not NULL
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }
    
    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    debug_hook_sp = this_sp;

    /* Determine if we are in dualsp mode or not.  If we are in dualsp
     * mode set the debug hook on `this' SP only since the other SP
     * maybe in the reset state.
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        debug_hook_sp = this_sp;
    }

    /* Validate the rebuild hook which are located after any debug hooks.
     */
    if ((sep_params_p == NULL)                                                                 ||
        (sep_params_p->scheduler_hooks[rebuild_hook_index].object_id == 0)                     ||
        (sep_params_p->scheduler_hooks[rebuild_hook_index].object_id == FBE_OBJECT_ID_INVALID)    )
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Rebuild hook: %p index: %d unexpected", 
                   __FUNCTION__, sep_params_p, rebuild_hook_index);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }

    /* Set the transport server to the correct SP.
     */
    status = fbe_api_sim_transport_set_target_server(debug_hook_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize complete array.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        raid_groups_complete[rg_index] = FBE_FALSE;
    }

    /*  Loop until we either see all the proactive copies start or timeout
     */
    for (retry_count = 0; (retry_count < max_retries) && (num_raid_groups_complete < raid_group_count); retry_count++)
    {
        /* For all raid groups simply check */
        current_rg_config_p = rg_config_p;
        for (rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            /* Skip raid groups that have completed the event.
             */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }

            /* Skip raid groups that are not enabled.
             */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                raid_groups_complete[rg_index] = FBE_TRUE;
                num_raid_groups_complete++;
                current_rg_config_p++;
                continue;
            }

            /* Validate sparing position.
             */
            if ((position_to_copy == FBE_TEST_SEP_BACKGROUND_PAUSE_INVALID_DISK_POSITION) ||
                (position_to_copy >= current_rg_config_p->width)                             )
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid copy position: %d",
                           __FUNCTION__, position_to_copy);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                status = fbe_api_sim_transport_set_target_server(this_sp);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return FBE_STATUS_GENERIC_FAILURE; 
            }

            /* Get the vd object id of the position to force into proactive copy
             */
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

            /* Check if we have reached the debug hook.
             */
            rebuild_hook_index = rg_index;
            status = fbe_test_sep_background_pause_check_debug_hook(&sep_params_p->scheduler_hooks[rebuild_hook_index],
                                                                    &b_is_debug_hook_reached);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

            /* If we have reached the debug hook log a message.
             */
            if (b_is_debug_hook_reached == FBE_TRUE)
            {
                /* Flag the debug hook reached.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "fbe_test_sep_background_pause: Hit hook raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                           rg_object_id, position_to_copy, vd_object_id,
                           drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);
                raid_groups_complete[rg_index] = FBE_TRUE;
                num_raid_groups_complete++;
            }

            /* Goto next raid group
             */                
            current_rg_config_p++;

        } /* end for all raid groups */

        /* If not done sleep for a short period
         */
        if (num_raid_groups_complete < raid_group_count)
        {
            fbe_api_sleep(FBE_TEST_SEP_BACKGROUND_PAUSE_SHORT_RETRY_TIME);
        }

    } /* end while not complete or retries exhausted */

    /* Check if we are not complete.
     */
    if ((retry_count >= max_retries)                  || 
        (num_raid_groups_complete < raid_group_count)    )
    {
        /* Go thru all the raid groups and report the ones that timed out.
         */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            /* Skip raid groups that are not enabled.
             */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                current_rg_config_p++;
                continue;
            }

            /* Skip raid groups that have completed the event.
             */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }

            /* Using the position supplied, get the virtual drive information.
             */
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

            /* Display some information
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "fbe_test_sep_background_pause: Timed out rebuild hook: raid: 0x%x pos: %d vd: 0x%x (%d_%d_%d)",
                       rg_object_id, position_to_copy, vd_object_id,
                       drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);

            /* Goto the next raid group
             */
            current_rg_config_p++;

        } /* For all raid groups */

        /* Print a message then fail.
         */
        status = FBE_STATUS_TIMEOUT;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: %d raid groups out of: %d took longer than: %d seconds to hit rebuild hook",
                   (raid_group_count - num_raid_groups_complete), raid_group_count, 
                   ((FBE_TEST_SEP_BACKGROUND_PAUSE_MAX_SHORT_RETRIES * FBE_TEST_SEP_BACKGROUND_PAUSE_SHORT_RETRY_TIME) / 1000));

        /* Generate an error trace on the SP and fail the test.
         */
        fbe_test_sep_util_raid_group_generate_error_trace(rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set the SP back to the original.
         */
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Wait for the associated virtual drive to be READY before we proceed.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
    
        /* Using the position supplied, get the virtual drive information.
         */
        drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Wait for the virtual drive to be ready before continuing.
         */
        status = fbe_test_common_util_wait_for_object_lifecycle_state_on_sp(vd_object_id, 
                                                                            debug_hook_sp,
                                                                            FBE_LIFECYCLE_STATE_READY, 
                                                                            20000, 
                                                                            FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Set the transport server to `this SP'.
     */
    status = fbe_api_sim_transport_set_target_server(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/******************************************************************
 * end fbe_test_sep_background_pause_wait_for_hook()
 ******************************************************************/

/*!***************************************************************************
 *          fbe_test_sep_background_pause_wait_for_encryption_state()
 *****************************************************************************
 *
 * @brief   Wait until the associated virtual drive(s) changes it's encryption
 *          state to the desired state.
 *
 * @param   rg_config_p - Array of raid group information  
 * @param   raid_group_count - Number of valid raid groups in array 
 * @param   position_to_copy - The position in the raid group to copy from
 * @param   expected_encryption_state - The encryption state expexcted.
 * @param   sep_params_p - Pointer to 
 * @param   b_is_reboot_test - FBE_TRUE - Need to save hooks across reboot
 *
 * @return  fbe_status_t 
 *
 * @author
 *  06/09/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_test_sep_background_pause_wait_for_encryption_state(fbe_test_rg_configuration_t *rg_config_p,
                                                                     fbe_u32_t raid_group_count,
                                                                     fbe_u32_t position_to_copy,
                                                                     fbe_base_config_encryption_state_t expected_encryption_state)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u32_t                               rg_index;
    fbe_test_rg_configuration_t            *current_rg_config_p = rg_config_p;
    fbe_bool_t                              b_is_dualsp_mode = FBE_FALSE;
    fbe_sim_transport_connection_target_t   this_sp;
    fbe_sim_transport_connection_target_t   other_sp;
    fbe_sim_transport_connection_target_t   debug_hook_sp;
    fbe_object_id_t                         rg_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         vd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t                               retry_count;
    fbe_u32_t                               max_retries = FBE_TEST_SEP_BACKGROUND_PAUSE_MAX_SHORT_RETRIES;
    fbe_bool_t                              raid_groups_complete[FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT];
    fbe_u32_t                               num_raid_groups_complete = 0;
    fbe_test_raid_group_disk_set_t         *drive_to_spare_p = NULL; 
    fbe_base_config_encryption_state_t      current_encryption_state;

    /*! @todo Currently no supported on hardware.
     */
    if (fbe_test_util_is_simulation() == FBE_FALSE)
    {
        /*! @todo This test is not currently supported on hardware.
         */
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Forcing proactive sparing not supported on hardware.", 
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Validate the destination array is not NULL
     */
    if (raid_group_count > FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "%s: Number of raid groups: %d is greater than max: %d", 
                   __FUNCTION__, raid_group_count, FBE_TEST_SEP_BACKGROUND_OPS_MAX_RAID_GROUP_COUNT);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
        return status;
    }
    
    /* Get `this' and other SP.
     */
    this_sp = fbe_api_sim_transport_get_target_server();
    other_sp = (this_sp == FBE_SIM_SP_A) ? FBE_SIM_SP_B : FBE_SIM_SP_A;
    debug_hook_sp = this_sp;

    /* Determine if we are in dualsp mode or not.  If we are in dualsp
     * mode set the debug hook on `this' SP only since the other SP
     * maybe in the reset state.
     */
    b_is_dualsp_mode = fbe_test_sep_util_get_dualsp_test_mode();
    if (b_is_dualsp_mode == FBE_TRUE)
    {
        debug_hook_sp = this_sp;
    }

    /* Set the transport server to the correct SP.
     */
    status = fbe_api_sim_transport_set_target_server(debug_hook_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    /* Initialize complete array.
     */
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        raid_groups_complete[rg_index] = FBE_FALSE;
    }

    /*  Loop until we either see all the proactive copies start or timeout
     */
    for (retry_count = 0; (retry_count < max_retries) && (num_raid_groups_complete < raid_group_count); retry_count++)
    {
        /* For all raid groups simply check */
        current_rg_config_p = rg_config_p;
        for (rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            /* Skip raid groups that have completed the event.
             */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }

            /* Skip raid groups that are not enabled.
             */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                raid_groups_complete[rg_index] = FBE_TRUE;
                num_raid_groups_complete++;
                current_rg_config_p++;
                continue;
            }

            /* Validate sparing position.
             */
            if ((position_to_copy == FBE_TEST_SEP_BACKGROUND_PAUSE_INVALID_DISK_POSITION) ||
                (position_to_copy >= current_rg_config_p->width)                             )
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                mut_printf(MUT_LOG_TEST_STATUS, "%s Invalid copy position: %d",
                           __FUNCTION__, position_to_copy);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

                status = fbe_api_sim_transport_set_target_server(this_sp);
                MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
                return FBE_STATUS_GENERIC_FAILURE; 
            }

            /* Get the vd object id of the position to force into proactive copy
             */
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

            /* Check if we have reached the encryption state
             */
            status = fbe_api_base_config_get_encryption_state(vd_object_id, &current_encryption_state);
            MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);
            if (current_encryption_state == expected_encryption_state) 
            {
                /* Flag the debug hook reached.
                 */
                mut_printf(MUT_LOG_TEST_STATUS, 
                           "fbe_test_sep_background_pause: Hit encryption state: %d raid obj: 0x%x pos: %d vd obj: 0x%x (%d_%d_%d)",
                           current_encryption_state, rg_object_id, position_to_copy, vd_object_id,
                           drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);
                raid_groups_complete[rg_index] = FBE_TRUE;
                num_raid_groups_complete++;
            }

            /* Goto next raid group
             */                
            current_rg_config_p++;

        } /* end for all raid groups */

        /* If not done sleep for a short period
         */
        if (num_raid_groups_complete < raid_group_count)
        {
            fbe_api_sleep(FBE_TEST_SEP_BACKGROUND_PAUSE_SHORT_RETRY_TIME);
        }

    } /* end while not complete or retries exhausted */

    /* Check if we are not complete.
     */
    if ((retry_count >= max_retries)                  || 
        (num_raid_groups_complete < raid_group_count)    )
    {
        /* Go thru all the raid groups and report the ones that timed out.
         */
        current_rg_config_p = rg_config_p;
        for ( rg_index = 0; rg_index < raid_group_count; rg_index++)
        {
            /* Skip raid groups that are not enabled.
             */
            if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
            {
                current_rg_config_p++;
                continue;
            }

            /* Skip raid groups that have completed the event.
             */
            if (raid_groups_complete[rg_index] == FBE_TRUE)
            {
                current_rg_config_p++;
                continue;
            }

            /* Using the position supplied, get the virtual drive information.
             */
            drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
            fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
            status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
            MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
            MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

            /* Display some information
             */
            mut_printf(MUT_LOG_TEST_STATUS, 
                       "fbe_test_sep_background_pause: Timed out wait for encryption state: %d raid: 0x%x pos: %d vd: 0x%x (%d_%d_%d)",
                       expected_encryption_state, rg_object_id, position_to_copy, vd_object_id,
                       drive_to_spare_p->bus, drive_to_spare_p->enclosure, drive_to_spare_p->slot);

            /* Goto the next raid group
             */
            current_rg_config_p++;

        } /* For all raid groups */

        /* Print a message then fail.
         */
        status = FBE_STATUS_TIMEOUT;
        mut_printf(MUT_LOG_TEST_STATUS, 
                   "fbe_test_sep_background_pause: %d raid groups out of: %d took longer than: %d seconds to hit encryption state",
                   (raid_group_count - num_raid_groups_complete), raid_group_count, 
                   ((FBE_TEST_SEP_BACKGROUND_PAUSE_MAX_SHORT_RETRIES * FBE_TEST_SEP_BACKGROUND_PAUSE_SHORT_RETRY_TIME) / 1000));

        /* Generate an error trace on the SP and fail the test.
         */
        fbe_test_sep_util_raid_group_generate_error_trace(rg_object_id);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Set the SP back to the original.
         */
        status = fbe_api_sim_transport_set_target_server(this_sp);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Wait for the associated virtual drive to be READY before we proceed.
     */
    current_rg_config_p = rg_config_p;
    for (rg_index = 0; rg_index < raid_group_count; rg_index++)
    {
        if (!fbe_test_rg_config_is_enabled(current_rg_config_p))
        {
            current_rg_config_p++;
            continue;
        }
    
        /* Using the position supplied, get the virtual drive information.
         */
        drive_to_spare_p = &current_rg_config_p->rg_disk_set[position_to_copy];
        fbe_api_database_lookup_raid_group_by_number(current_rg_config_p->raid_group_id, &rg_object_id);
        status = fbe_test_sep_util_get_virtual_drive_object_id_by_position(rg_object_id, position_to_copy, &vd_object_id);
        MUT_ASSERT_INT_EQUAL(status, FBE_STATUS_OK);
        MUT_ASSERT_INT_NOT_EQUAL(vd_object_id, FBE_OBJECT_ID_INVALID);

        /* Wait for the virtual drive to be ready before continuing.
         */
        status = fbe_test_common_util_wait_for_object_lifecycle_state_on_sp(vd_object_id, 
                                                                            debug_hook_sp,
                                                                            FBE_LIFECYCLE_STATE_READY, 
                                                                            20000, 
                                                                            FBE_PACKAGE_ID_SEP_0);
        MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

        /* Goto next raid group.
         */
        current_rg_config_p++;
    }

    /* Set the transport server to `this SP'.
     */
    status = fbe_api_sim_transport_set_target_server(this_sp);
    MUT_ASSERT_INT_EQUAL(FBE_STATUS_OK, status);

    return status;
}
/******************************************************************
 * end fbe_test_sep_background_pause_wait_for_encryption_state()
 ******************************************************************/


/******************************************************
 * end of file fbe_test_sep_background_pause.c
 ******************************************************/

