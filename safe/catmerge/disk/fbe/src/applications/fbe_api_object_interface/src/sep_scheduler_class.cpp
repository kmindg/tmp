/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2011
 * All Rights Reserved.
 * Licensed Material-Property of EMC Corporation.
 * This software is made available solely pursuant to the terms
 * of a EMC license agreement which governs its use.
 *********************************************************************/

/*********************************************************************
 *
 *  Description:
 *      This file defines the methods of the SEP SCHEDULER INTERFACE class.
 *
 *  Notes:
 *      The SEP Scheduler class (sepScheduler) is a derived class of
 *      the base class (bSEP).
 *
 *  History:
 *      09-Sep-2011 : initial version
 *
 *********************************************************************/

#ifndef T_SEP_SCHEDULER_CLASS_H
#include "sep_scheduler_class.h"
#endif

/*********************************************************************
* sepScheduler Class :: sepScheduler() (Constructor)
*********************************************************************
*  Description:
*       Initializes class variables
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/
sepScheduler::sepScheduler() : bSEP()
{
    // Store Object number
    idnumber = (unsigned) SEP_SCHEDULER_INTERFACE;
    sepSchedulerCount = ++gSepSchedulerCount;

    // Store Object name
    idname = new char [IDNAME_LENGTH];
    strcpy(idname, "SEP_SCHEDULER_INTERFACE");
    Sep_Scheduler_Intializing_variable();

    if (Debug) {
        sprintf(temp, "sepScheduler class constructor (%d) %s\n",
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SEP sepScheduler Interface function calls>
    sepSchedulerInterfaceFuncs=
        " [function call]\n" \
        " [short name]         [FBE API Scheduler Interface]\n"\
        " -------------------------------------------------------------------------\n"\
        " addDebugHook           fbe_api_scheduler_add_debug_hook\n"\
        " delDebugHook           fbe_api_scheduler_del_debug_hook\n"\
        " getDebugHook           fbe_api_scheduler_get_debug_hook\n"\
        " clearAllDebugHooks     fbe_api_scheduler_clear_all_debug_hooks\n"\
        " -------------------------------------------------------------------------\n";

    // Define common usage for SEP Scheduler commands
       usage_format = " Usage: %s  [object_id]  [monitor_state]"\
        "  [monitor_substate]  [val1]  [val2]  "\
        "[check_type]  [action]\n"\
        " For example: %s 0X10E SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY NULL NULL SCHEDULER_CHECK_STATES SCHEDULER_DEBUG_ACTION_LOG\n\n"\
        "Valid values for monitor state:\n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD                      \n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD             \n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY                       \n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_VERIFY              \n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR                 \n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL                     \n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL                      \n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE                     \n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN                         \n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL             \n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_START \n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_EXPIRE\n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY   \n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH    \n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REBUILD   n"\
        " SCHEDULER_MONITOR_STATE_RAID_GROUP_PENDING                          \n"\
        " SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_JOIN              \n"\
        " SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO              \n"\
        " SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY      \n"\
        " SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_QUIESCE           \n"\
        " SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VERIFY_INVALIDATE \n"\
        " SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_BLOCK_OPERATION \n"\
        " SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EOL           \n"\
        " SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_DOWNLOAD_PERMISSION           \n"\
        " SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED                        \n"\
        " SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN         \n"\
        " SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY                          \n"\
        " SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY            \n"\
        " SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT        \n"\
        " SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE                      \n"\
        " SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE\n"\
        " SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_PERSIST\n"\
        " SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT       \n"\
        " SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_NON_PAGED_INIT\n"\
        " SCHEDULER_MONITOR_STATE_LUN_INCOMPLETE_WRITE_VERIFY\n"\
        " SCHEDULER_MONITOR_STATE_LUN_ZERO      \n\n"\
        "Valid values for monitor substate: \n"\
        " FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY                    \n"\
        " FBE_RAID_GROUP_SUBSTATE_REBUILD_SINGLY_DEGRADED          \n"\
        " FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_DOWNSTREAM_PERMISSION \n"\
        " FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_UPSTREAM_PERMISSION   \n"\
        " FBE_RAID_GROUP_SUBSTATE_REBUILD_DOWNSTREAM_HIGHER_PRIORITY        \n"\
        " FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_PERMISSION            \n"\
        " FBE_RAID_GROUP_SUBSTATE_REBUILD_SEND                     \n"\
        " FBE_RAID_GROUP_SUBSTATE_REBUILD_COMPLETED                \n"\
        " FBE_RAID_GROUP_SUBSTATE_JOIN_REQUEST                     \n"\
        " FBE_RAID_GROUP_SUBSTATE_JOIN_STARTED                     \n"\
        " FBE_RAID_GROUP_SUBSTATE_JOIN_DONE                        \n"\
        " FBE_RAID_GROUP_SUBSTATE_JOIN_SYNCED                      \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_JOIN_REQUEST                \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_JOIN_STARTED                \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_JOIN_DONE                   \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_JOIN_SYNCED                 \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY                  \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION             \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_QUIESCE_ENTRY               \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_VERIFY_INVALIDATE_CHECKPOINT\n"
        " FBE_PROVISION_DRIVE_SUBSTATE_REMAP_EVENT              \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_BLOCK_OPERATION_CHECK   \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_NON_MCR_REGION_CHECKPOINT\n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_SET_EOL         \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_CLEAR_EOL       \n"\
        " FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED                         \n"\
        " FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED    \n"\
        " FBE_VIRTUAL_DRIVE_SUBSTATE_MIRROR_MODE_SET   \n"\
        " FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE     \n"\
        " FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_DISCONNECTED \n"\
        " FBE_VIRTUAL_DRIVE_SUBSTATE_PASS_THRU_MODE_SET\n"\
        " FBE_VIRTUAL_DRIVE_SUBSTATE_SET_PASS_THRU_CONDITION           \n"\
        " FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE                \n"\
        " FBE_VIRTUAL_DRIVE_SUBSTATE_PASS_THRU_MODE_SET                       \n"\
        " FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_SWAP_OUT                        \n"\
        " FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE                      \n"\
        " FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_DISCONNECTED                      \n"\
        " FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY                                  \n"\
        " FBE_VIRTUAL_DRIVE_SUBSTATE_BROKEN                                   \n"\
        " FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST      \n"\
        " FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED      \n"\
        " FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE         \n"\
        " FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED2     \n"\
        " FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN\n"\
        " FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST    \n"\
        " FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED    \n"\
        " FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_DONE       \n"\
        " FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED2   \n"\
        " FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_REQUEST \n"\
        " FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_STARTED\n"\
        " FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE\n"\
        " FBE_RAID_GROUP_SUBSTATE_GOING_BROKEN  \n"\
        " FBE_RAID_GROUP_SUBSTATE_METADATA_MARK_RECONSTRUCT \n"\
        " FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_STARTED\n"\
        " FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_ERROR \n"\
        " FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT     \n"\
        " FBE_RAID_GROUP_SUBSTATE_START_METADATA_VERIFY  \n"\
        " FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_INCOMPLETE_WRITE \n"\
        " FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_NEEDS_REBUILD \n"\
        " FBE_RAID_GROUP_SUBSTATE_VERIFY_NO_PERMISSION \n"\
        " FBE_RAID_GROUP_SUBSTATE_VERIFY_SEND      \n"\
        " FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RO_START\n"\
        " FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RW_START \n"\
        " FBE_RAID_GROUP_SUBSTATE_VERIFY_INCOMPLETE_WRITE_START\n"\
        " FBE_RAID_GROUP_SUBSTATE_VERIFY_SYSTEM_START\n"\
        " FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START \n"\
        " FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED\n"\
        " FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_REQUEST \n"\
        " FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_STARTED\n"\
        " FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_CLEARED\n"\
        " FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_PERSIST \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_QUEUED  \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_COMPLETED \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_NO_PERMISSION \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_SEND_VERIFY \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_ZERO_NO_PERMISSION\n"\
        " FBE_LUN_SUBSTATE_INCOMPLETE_WRITE_VERIFY_REQUESTED\n"\
        " FBE_LUN_SUBSTATE_ZERO_CHECKPOINT_UPDATED \n"\
        " FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_START\n"\
        " FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_DONE \n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_NOT_FIRST_CREATE\n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_NONPAGED_INIT\n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_FIRST_CREATE\n"\
        " FBE_PROVISION_DRIVE_SUBSTATE_DOWNLOAD_PERMISSION \n\n"\
        "Valid values for Check:\n"\
        " SCHEDULER_CHECK_STATES\n"\
        " SCHEDULER_CHECK_VALS_EQUAL\n"\
        " SCHEDULER_CHECK_VALS_LT\n\n"\
        " SCHEDULER_CHECK_VALS_GT                                            \n\n"\
        "Valid values for Action:\n"\
        " SCHEDULER_DEBUG_ACTION_LOG\n"\
        " SCHEDULER_DEBUG_ACTION_CONTINUE\n"\
        " SCHEDULER_DEBUG_ACTION_PAUSE\n"\
        " SCHEDULER_DEBUG_ACTION_CLEAR\n";

}

/*********************************************************************
* sepScheduler Class :: MyIdNumIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id number of object
*  Input: Parameters
*      none
*  Returns:
*      unsigned int - id number
*********************************************************************/

unsigned sepScheduler::MyIdNumIs(void)
{
    return idnumber;
};

/*********************************************************************
* sepScheduler Class :: MyIdNameIs() (Accessor method)
*********************************************************************
*  Description:
*       Returns id name of object
*  Input: Parameters
*      none
*  Returns:
*      char* - id name
*********************************************************************/

char * sepScheduler::MyIdNameIs(void)
{
    return idname;
};

/*********************************************************************
* sepScheduler Class :: dumpme()
*********************************************************************
*  Description:
*       Dumps id number and sep Scheduler count
*  Input: Parameters
*      none
*  Returns:
*      none
*********************************************************************/

void sepScheduler::dumpme(void)
{
    strcpy (key, "sepScheduler::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n",
       idnumber, sepSchedulerCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* sepScheduler Class :: select(int index, int argc, char * argv [ ])
*********************************************************************
*
*  Description:
*      This function looks up the function short name to validate
*      it and then calls the function with the index passed back
*      from the compare.
*
*  Input: Parameters
*      index - index into arguments
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*
*  History
*      09-Sep-2011 : initial version
*
*********************************************************************/
fbe_status_t sepScheduler::select (int index, int argc, char * argv [ ])
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SEP interface");

    // Display all SEP Scheduler functions and return generic error when
    // the argument count is less than 6 and no help command executed.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sepSchedulerInterfaceFuncs);
        return status;
    }

    // add scheduler debug hook
    if (strcmp (argv[index], "ADDDEBUGHOOK") == 0) {
        status = add_debug_hook(argc, &argv[index]);

    // delete scheduler debug hook
    } else if (strcmp (argv[index], "DELDEBUGHOOK") == 0) {
        status = delete_debug_hook(argc, &argv[index]);

    // get scheduler debug hook
    } else if (strcmp (argv[index], "GETDEBUGHOOK") == 0) {
        status =  get_debug_hook(argc, &argv[index]);

    // clear all scheduler debug hook
    } else if (strcmp (argv[index], "CLEARALLDEBUGHOOKS") == 0) {
        status = clear_all_debug_hooks(argc, &argv[index]);
    // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n",
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sepSchedulerInterfaceFuncs );
    }
    return status;
}

/*********************************************************************
* sepScheduler Class :: add_debug_hook(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Set the debug hook in the scheduler for given object id.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*
*  History
*     09-Sep-2011 : initial version
*********************************************************************/
fbe_status_t sepScheduler::add_debug_hook(
    int argc, char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "addDebugHook",
        "sepScheduler::add_debug_hook",
        "fbe_api_scheduler_add_debug_hook",
        usage_format,
        argc, 13);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Extract the arguments
    status = extract_scheduler_debug_hook_args(argv);

    sprintf(params, "object_id(0x%x)  "\
        "monitor_state(%s)  "\
        "monitor_substate(%s)  "\
        "val1(0x%llx)  "\
        "val2(0x%llx)  "\
        "check_type(%s)  "\
        "action(%s)", 
        hook.object_id,
        edit_monitor_state(hook.monitor_state), 
        edit_monitor_substate(hook.monitor_substate),
        (unsigned long long)hook.val1,
        (unsigned long long)hook.val2,
        edit_check_type(hook.check_type),
        edit_action(hook.action));
    
    if(status !=FBE_STATUS_OK){
        sprintf(temp,"Failed to set debug hook for object id %x.\n", hook.object_id);
         call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call:
    status = fbe_api_scheduler_add_debug_hook(hook.object_id,
        hook.monitor_state, 
        hook.monitor_substate,
        hook.val1,
        hook.val2,
        hook.check_type,
        hook.action);

    // Check status of call
    if (status == FBE_STATUS_OK)
    {
        sprintf(temp,"Successfully added debug hook for object id 0X%x.\n",hook.object_id);
    } else{
        sprintf(temp,"Failed to set debug hook for object id 0X%x.\n", hook.object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepScheduler Class :: delete_debug_hook(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Delete the debug hook in the scheduler for given object id.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*
*  History
*     09-Sep-2011 : initial version
*********************************************************************/
fbe_status_t sepScheduler::delete_debug_hook(
    int argc, char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "delDebugHook",
        "sepScheduler::delete_debug_hook",
        "fbe_api_scheduler_del_debug_hook",
        usage_format,
        argc, 13);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // Extract the arguments
    status = extract_scheduler_debug_hook_args(argv);

    sprintf(params, "object_id(0x%x)  "\
        "monitor_state(%s)  "\
        "monitor_substate(%s)  "\
        "val1(0x%llx)  "\
        "val2(0x%llx)  "\
        "check_type(%s)  "\
        "action(%s)", 
        hook.object_id,
        edit_monitor_state(hook.monitor_state), 
        edit_monitor_substate(hook.monitor_substate),
        (unsigned long long)hook.val1,
        (unsigned long long)hook.val2,
        edit_check_type(hook.check_type),
        edit_action(hook.action));

    if(status !=FBE_STATUS_OK){
        sprintf(temp,"Failed to delete debug hook for object id %d\n",hook.object_id);
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call:
    status = fbe_api_scheduler_del_debug_hook(hook.object_id,
        hook.monitor_state, 
        hook.monitor_substate,
        hook.val1,
        hook.val2,
        hook.check_type,
        hook.action);

    // Check status of call
    if (status == FBE_STATUS_OK)
    {
        sprintf(temp,"Successfully deleted debug hook for object id 0X%x\n",hook.object_id);
    } else{
        sprintf(temp,"Failed to delete debug hook for object id 0X%x\n",hook.object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepScheduler Class :: get_debug_hook(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Get the debug hook in the scheduler for given object id.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*
*  History
*     09-Sep-2011 : initial version
*********************************************************************/
fbe_status_t sepScheduler::get_debug_hook(
    int argc, char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getDebugHook",
        "sepScheduler::get_debug_hook",
        "fbe_api_scheduler_get_debug_hook",
        usage_format,
        argc, 13);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    // Extract the arguments
     status = extract_scheduler_debug_hook_args(argv);

     sprintf(params, "object_id(%x)  "\
        "monitor_state(%s)  "\
        "monitor_substate(%s)  "\
        "val1(0x%llx)  "\
        "val2(0x%llx)  "\
        "check_type(%s)  "\
        "action(%s)", 
        hook.object_id,
        edit_monitor_state(hook.monitor_state), 
        edit_monitor_substate(hook.monitor_substate),
        (unsigned long long)hook.val1,
        (unsigned long long)hook.val2,
        edit_check_type(hook.check_type),
        edit_action(hook.action));

    if(status !=FBE_STATUS_OK){
        sprintf(temp,"Could not get the debug hook information "
            "for object id 0X%X\n",hook.object_id);
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call:
    status =  fbe_api_scheduler_get_debug_hook(&hook);

    // Check status of call
    if (status == FBE_STATUS_OK)
    {
        sprintf(temp,"\n"\
            "<Counter>       %lld\n",
            (long long)hook.counter);

    } else {
        sprintf(temp,"Could not get the debug hook information "
            "for object id 0X%x.\n", hook.object_id);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}

/*********************************************************************
* sepScheduler Class ::extract_scheduler_debug_hook_args(char**  argv)
*********************************************************************
*
*  Description:
*      Gets the input from command prompt.
*
*  Input: Parameters
*      *argv - pointer to arguments
*
*  Returns:
*      void
*
*  History
*     19-Sep-2011 : initial version
*********************************************************************/
fbe_status_t sepScheduler ::extract_scheduler_debug_hook_args(char**  argv){
    //Extract the object id
    argv++;
    hook.object_id = (fbe_u32_t)strtoul(*argv, 0, 0); 
    
    //Extract the monitor state
    argv++; 
    sep_scheduler_get_monitor_state_by_string(argv);
    monitor_state_status = verify_monitor_state(hook.monitor_state);
    
    //Extract the monitor substate
    argv++;
    sep_scheduler_get_monitor_substate_by_string(argv);
    monitor_substate_status = verify_monitor_substate(hook.monitor_substate);

    //Extract the val1
    argv++;
    hook.val1= strtoul(*argv, 0, 0);

    //Extract the val2
    argv++;
    hook.val2 = strtoul(*argv, 0, 0);

    //Extract the check type
    argv++;
    sep_scheduler_get_check_type_as_string(argv); 
    check_type_status = verify_check_type(hook.check_type);

    //Extract the action
    argv++;
    sep_scheduler_get_action_as_string(argv);
    action_status = verify_action(hook.action);

    //return status
    if(monitor_state_status!=FBE_STATUS_OK || 
        monitor_substate_status != FBE_STATUS_OK || 
        check_type_status !=FBE_STATUS_OK ||
        action_status !=FBE_STATUS_OK)
    {
    return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*********************************************************************
* sepScheduler Class ::verify_monitor_state( fbe_u32_t monitor_state)
*********************************************************************
*
*  Description:
*      Verify the monitor state.
*
*  Input: Parameters
*      monitor_state
*
*  Returns:
*      fbe_status_t
*
*  History
*     19-Sep-2011 : initial version
*********************************************************************/
fbe_status_t sepScheduler::verify_monitor_state( fbe_u32_t monitor_state){

    if((monitor_state == SCHEDULER_MONITOR_STATE_INVALID)||
        (monitor_state >= SCHEDULER_MONITOR_STATE_LAST)){
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else{
        status = FBE_STATUS_OK;
    }
    
    return status;
}

/*********************************************************************
* sepScheduler Class ::verify_monitor_substate( fbe_u32_t monitor_substate)
*********************************************************************
*
*  Description:
*      Verify the monitor substate.
*
*  Input: Parameters
*      monitor_substate
*
*  Returns:
*      fbe_status_t
*
*  History
*     19-Sep-2011 : initial version
*********************************************************************/

fbe_status_t sepScheduler::verify_monitor_substate( 
    fbe_u32_t monitor_substate)
{

    if((monitor_substate ==FBE_SCHEDULER_MONITOR_SUB_STATE_INVALID) ||
        (monitor_substate >= FBE_SCHEDULER_MONITOR_SUB_STATE_LAST) ){
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else{
        status = FBE_STATUS_OK;
    }
    return status;
}

/*********************************************************************
* sepScheduler Class ::verify_check_type(fbe_u32_t check_type)
*********************************************************************
*
*  Description:
*      Verify the check type.
*
*  Input: Parameters
*      check_type
*
*  Returns:
*      fbe_status_t
*
*  History
*     19-Sep-2011 : initial version
*********************************************************************/

fbe_status_t sepScheduler::verify_check_type(fbe_u32_t check_type) {
    switch(check_type){
        case SCHEDULER_CHECK_STATES:
        case SCHEDULER_CHECK_VALS_EQUAL:
        case SCHEDULER_CHECK_VALS_GT:
        case  SCHEDULER_CHECK_VALS_LT:
            status = FBE_STATUS_OK;
            break;

        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return status;
}

/*********************************************************************
* sepScheduler Class ::verify_action(fbe_u32_t action)
*********************************************************************
*
*  Description:
*      Verify the action.
*
*  Input: Parameters
*      action
*
*  Returns:
*      fbe_status_t
*
*  History
*     19-Sep-2011 : initial version
*********************************************************************/

fbe_status_t sepScheduler::verify_action(fbe_u32_t action){
    switch(action){
        case SCHEDULER_DEBUG_ACTION_LOG:
            status = FBE_STATUS_OK;
            break;

        case SCHEDULER_DEBUG_ACTION_CONTINUE :
            status = FBE_STATUS_OK;
            break;

        case SCHEDULER_DEBUG_ACTION_PAUSE:
            status = FBE_STATUS_OK;
            break;

        case SCHEDULER_DEBUG_ACTION_CLEAR:
            status = FBE_STATUS_OK;
            break;

        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

    }
    return status;
}

/*********************************************************************
* sepScheduler Class ::edit_monitor_state(fbe_u32_t monitor_state)
*********************************************************************
*
*  Description:
*      Edit the monitor state.
*
*  Input: Parameters
*      fbe_u32_t monitor_state
*
*  Returns:
*      char*
*
*  History
*     22-Sep-2011 : initial version
*********************************************************************/
char*  sepScheduler::edit_monitor_state(fbe_u32_t monitor_state)
{
    switch(monitor_state)
    {
        case SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD");
            break;

        case SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD");
            break;

        case SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY");
            break;

        case SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_VERIFY:
         sprintf(mon_state, "SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_VERIFY,");
         break;
        
        case SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR");
            break;

        case SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL");
            break;

        case SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL");
            break;

        case SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE");
            break;

        case SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN");
            break;
            
        case SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL");
            break;

        case SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_START:
            sprintf(mon_state, "SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_START");
            break;
            
        case SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_EXPIRE:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_EXPIRE");
            break;

        case SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY");
            break;

        case SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH");
            break;

        case SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REBUILD:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REBUILD");
            break;

        case SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_JOIN:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_JOIN");
            break;

        case SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO");
            break;

        case SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY");
            break;

        case SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_QUIESCE:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_QUIESCE");
            break;
            
        case SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VERIFY_INVALIDATE:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VERIFY_INVALIDATE");
            break;

        case SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_BLOCK_OPERATION:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_BLOCK_OPERATION");
            break;

        case SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EOL:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EOL");
            break;

        case SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_DOWNLOAD_PERMISSION:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_DOWNLOAD_PERMISSION");
            break;

        case SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION");
            break;
                  
        case SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED:
            sprintf(mon_state, "SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED");
            break;
        case SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN:
            sprintf(mon_state, "SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN");
            break;
        case SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY:
            sprintf(mon_state, "SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY");
            break;
        case SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY:
            sprintf(mon_state, "SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY");
            break;
        case SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT:
            sprintf(mon_state, "SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT");
            break;
        case SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE:
            sprintf(mon_state, "SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE");
            break;

        case SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_PERSIST:
            sprintf(mon_state, "SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_PERSIST");
            break;

        case SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE");
            break;

        case SCHEDULER_MONITOR_STATE_LUN_INCOMPLETE_WRITE_VERIFY:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_LUN_INCOMPLETE_WRITE_VERIFY");
            break;    
        case SCHEDULER_MONITOR_STATE_LUN_ZERO:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_LUN_ZERO");
            break;
        case SCHEDULER_MONITOR_STATE_LUN_CLEAN_DIRTY:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_LUN_CLEAN_DIRTY");
            break;

        case SCHEDULER_MONITOR_STATE_RAID_GROUP_PENDING:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_RAID_GROUP_PENDING");
            break;

        case SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT:
            sprintf(mon_state,"SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT");
            break;
	
        case SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_NON_PAGED_INIT:
            sprintf(mon_state, "SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_NON_PAGED_INIT");
	  break;

        default:
            sprintf(mon_state,"FBE_STATUS_GENERIC_FAILURE");
            break;

    }
    return mon_state;
}

/*********************************************************************
* sepScheduler Class ::edit_monitor_substate(fbe_u32_t monitor_substate)
*********************************************************************
*
*  Description:
*      Edit the monitor substate.
*
*  Input: Parameters
*      fbe_u32_t monitor_substate
*
*  Returns:
*      char*
*
*  History
*     22-Sep-2011 : initial version
*********************************************************************/
char* sepScheduler::edit_monitor_substate( 
    fbe_u32_t monitor_substate)
{
    switch(monitor_substate){
        case FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY:
            sprintf(mon_substate,"FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY");
            break;
        case FBE_RAID_GROUP_SUBSTATE_REBUILD_SINGLY_DEGRADED :
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_REBUILD_SINGLY_DEGRADED ");
            break;
        case FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_DOWNSTREAM_PERMISSION:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_DOWNSTREAM_PERMISSION");
            break;
        case FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_UPSTREAM_PERMISSION:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_UPSTREAM_PERMISSION");
            break;
        case FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_PERMISSION:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_PERMISSION");
            break;
        case FBE_RAID_GROUP_SUBSTATE_REBUILD_SEND:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_REBUILD_SEND");
            break;
        case FBE_RAID_GROUP_SUBSTATE_REBUILD_COMPLETED:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_REBUILD_COMPLETED");
            break;
        case FBE_RAID_GROUP_SUBSTATE_JOIN_REQUEST:
            sprintf(mon_substate,"FBE_RAID_GROUP_SUBSTATE_JOIN_REQUEST_ACTIVE");
            break;
        case FBE_RAID_GROUP_SUBSTATE_JOIN_STARTED:
            sprintf(mon_substate,"FBE_RAID_GROUP_SUBSTATE_JOIN_STARTED_ACTIVE");
            break;
        case FBE_RAID_GROUP_SUBSTATE_JOIN_DONE :
            sprintf(mon_substate,"FBE_RAID_GROUP_SUBSTATE_JOIN_DONE_ACTIVE");
            break;
        case FBE_RAID_GROUP_SUBSTATE_JOIN_SYNCED:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_JOIN_SYNCED");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_JOIN_REQUEST:
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_JOIN_REQUEST");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_JOIN_STARTED:
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_JOIN_STARTED");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_JOIN_DONE  :
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_JOIN_DONE  ");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_JOIN_SYNCED:
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_JOIN_SYNCED");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY:
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION :
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION ");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_QUIESCE_ENTRY:
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_QUIESCE_ENTRY");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_VERIFY_INVALIDATE_CHECKPOINT:
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_VERIFY_INVALIDATE_CHECKPOINT");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_REMAP_EVENT :
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_REMAP_EVENT ");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_BLOCK_OPERATION_CHECK:
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_BLOCK_OPERATION_CHECK");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_NON_MCR_REGION_CHECKPOINT:
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_NON_MCR_REGION_CHECKPOINT");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_SET_EOL :
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_SET_EOL ");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_CLEAR_EOL :
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_CLEAR_EOL ");
            break;
        case FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST");
            break;
        case FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED");
            break;
        case FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE :
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE ");
            break;
        case FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED2:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED2");
            break;
        case FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN");
            break;
        case FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST  :
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST  ");
            break;
        case FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED");
            break;
        case FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_DONE:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_DONE");
            break;
        case FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED2 :
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED2 ");
            break;
        case FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_REQUEST  :
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_REQUEST  ");
            break;
        case FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_STARTED:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_STARTED");
            break;
        case FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE :
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE ");
            break;
        case FBE_RAID_GROUP_SUBSTATE_GOING_BROKEN  :
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_GOING_BROKEN  ");
            break;
        case FBE_RAID_GROUP_SUBSTATE_METADATA_MARK_RECONSTRUCT:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_METADATA_MARK_RECONSTRUCT");
            break;
        case FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_STARTED:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_STARTED");
            break;
        case FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_ERROR:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_ERROR");
            break;
        case FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT :
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT ");
            break;
        case FBE_RAID_GROUP_SUBSTATE_START_METADATA_VERIFY  :
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_START_METADATA_VERIFY  ");
            break;
        case FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_INCOMPLETE_WRITE:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_INCOMPLETE_WRITE");
            break;
        case FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_NEEDS_REBUILD :
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_NEEDS_REBUILD ");
            break;
        case FBE_RAID_GROUP_SUBSTATE_VERIFY_NO_PERMISSION:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_VERIFY_NO_PERMISSION");
            break;
        case FBE_RAID_GROUP_SUBSTATE_VERIFY_SEND   :
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_VERIFY_SEND   ");
            break;
        case FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RO_START :
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RO_START ");
            break;
        case FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RW_START :
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RW_START ");
            break;
        case FBE_RAID_GROUP_SUBSTATE_VERIFY_INCOMPLETE_WRITE_START:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_VERIFY_INCOMPLETE_WRITE_START");
            break;
        case FBE_RAID_GROUP_SUBSTATE_VERIFY_SYSTEM_START:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_VERIFY_SYSTEM_START");
            break;
        case FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START");
            break;
        case FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED :
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED ");
            break;
        case FBE_RAID_GROUP_SUBSTATE_VERIFY_ALL_VERIFIES_COMPLETE:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_VERIFY_ALL_VERIFIES_COMPLETE ");
            break;
        case FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_REQUEST :
            sprintf(mon_substate, "FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_REQUEST ");
            break;
        case FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_STARTED:
            sprintf(mon_substate, "FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_STARTED");
            break;
        case FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_CLEARED :
            sprintf(mon_substate, "FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_CLEARED ");
            break;
        case FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_PERSIST :
            sprintf(mon_substate, "FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_PERSIST ");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_QUEUED:
            sprintf(mon_substate,"FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_QUEUED");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_COMPLETED:
            sprintf(mon_substate,"FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_COMPLETED");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_NO_PERMISSION :
            sprintf(mon_substate,"FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_NO_PERMISSION ");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_SEND_VERIFY :
            sprintf(mon_substate,"FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_SEND_VERIFY ");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND:
            sprintf(mon_substate,"FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_ZERO_NO_PERMISSION:
            sprintf(mon_substate,"FBE_PROVISION_DRIVE_SUBSTATE_ZERO_NO_PERMISSION");
            break;
        case FBE_LUN_SUBSTATE_INCOMPLETE_WRITE_VERIFY_REQUESTED:
            sprintf(mon_substate, "FBE_LUN_SUBSTATE_INCOMPLETE_WRITE_VERIFY_REQUESTED");
            break;
        case FBE_LUN_SUBSTATE_ZERO_CHECKPOINT_UPDATED:
            sprintf(mon_substate, "FBE_LUN_SUBSTATE_ZERO_CHECKPOINT_UPDATED");
            break;
        case FBE_LUN_SUBSTATE_LAZY_CLEAN_START:
            sprintf(mon_substate, "FBE_LUN_SUBSTATE_LAZY_CLEAN_START");
            break;
        case FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_START:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_START");
            break;
        case FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_DONE:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_DONE");
            break;
        case FBE_RAID_GROUP_SUBSTATE_REBUILD_DOWNSTREAM_HIGHER_PRIORITY:
            sprintf(mon_substate, "FBE_RAID_GROUP_SUBSTATE_REBUILD_DOWNSTREAM_HIGHER_PRIORITY");
            break;

        case FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED:
            sprintf(mon_substate, "FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED");
            break;
        case FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED:
            sprintf(mon_substate, "FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED");
            break;
        case FBE_VIRTUAL_DRIVE_SUBSTATE_MIRROR_MODE_SET:
            sprintf(mon_substate, "FBE_VIRTUAL_DRIVE_SUBSTATE_MIRROR_MODE_SET");
            break;
        case FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START:
            sprintf(mon_substate, "FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_START");
            break;
        case FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE:
            sprintf(mon_substate, "FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE");
            break;
        case FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_DENIED:
            sprintf(mon_substate, "FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_DENIED");
            break;
        case FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_GRANTED:
            sprintf(mon_substate, "FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_GRANTED");
            break;
        case FBE_VIRTUAL_DRIVE_SUBSTATE_SET_PASS_THRU_CONDITION:
            sprintf(mon_substate, "FBE_VIRTUAL_DRIVE_SUBSTATE_SET_PASS_THRU_CONDITION");
            break;
        case FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE:
            sprintf(mon_substate, "FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE");
            break;
        case FBE_VIRTUAL_DRIVE_SUBSTATE_PASS_THRU_MODE_SET:
            sprintf(mon_substate, "FBE_VIRTUAL_DRIVE_SUBSTATE_PASS_THRU_MODE_SET");
            break;
        case FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE:
            sprintf(mon_substate, "FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE");
            break;
        case FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_DISCONNECTED:
            sprintf(mon_substate, "FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_DISCONNECTED");
            break;
        case FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY:
            sprintf(mon_substate, "FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY");
            break;
        case FBE_VIRTUAL_DRIVE_SUBSTATE_BROKEN:
            sprintf(mon_substate, "FBE_VIRTUAL_DRIVE_SUBSTATE_BROKEN");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_NOT_FIRST_CREATE:
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_NOT_FIRST_CREATE");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_FIRST_CREATE:
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_FIRST_CREATE");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_DOWNLOAD_PERMISSION :
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_DOWNLOAD_PERMISSION ");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_NONPAGED_INIT:
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_NONPAGED_INIT");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START:
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_REQUEST:
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_REQUEST");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_SUCCESS:
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_SUCCESS");
            break;
        case FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_COMPLETE:
            sprintf(mon_substate, "FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_COMPLETE");
            break;

        default:
            sprintf(mon_substate,"FBE_STATUS_GENERIC_FAILURE");
            break;
    }
    return mon_substate;
}

/*********************************************************************
* sepScheduler Class ::edit_check_type(fbe_u32_t check_type)
*********************************************************************
*
*  Description:
*      Edit the check type.
*
*  Input: Parameters
*      fbe_u32_t check type
*
*  Returns:
*      char*
*
*  History
*     22-Sep-2011 : initial version
*********************************************************************/
char* sepScheduler::edit_check_type(fbe_u32_t check_type)
{

    switch(check_type){
        case SCHEDULER_CHECK_STATES:
            sprintf(check, "SCHEDULER_CHECK_STATES");
            break;

        case SCHEDULER_CHECK_VALS_EQUAL:
            sprintf(check, "SCHEDULER_CHECK_VALS_EQUAL");
            break;

        case  SCHEDULER_CHECK_VALS_LT:
            sprintf(check, "SCHEDULER_CHECK_VALS_LT");
            break;

        case SCHEDULER_CHECK_VALS_GT:
            sprintf(check,"SCHEDULER_CHECK_VALS_GT");
            break;
            
        default:
            sprintf(check, "FBE_STATUS_GENERIC_FAILURE");
            break;

    }
    return check;
}

/*********************************************************************
* sepScheduler Class ::edit_action(fbe_u32_t action)
*********************************************************************
*
*  Description:
*      Edit the action.
*
*  Input: Parameters
*      fbe_u32_t action
*
*  Returns:
*      char*
*
*  History
*     22-Sep-2011 : initial version
*********************************************************************/
char* sepScheduler::edit_action(fbe_u32_t action)
{
        switch(action){
        case SCHEDULER_DEBUG_ACTION_LOG:
            sprintf(act, "SCHEDULER_DEBUG_ACTION_LOG");
            break;

        case SCHEDULER_DEBUG_ACTION_CONTINUE :
            sprintf(act, "SCHEDULER_DEBUG_ACTION_CONTINUE");
            break;

        case SCHEDULER_DEBUG_ACTION_PAUSE:
            sprintf(act, "SCHEDULER_DEBUG_ACTION_PAUSE");
            break;

        case SCHEDULER_DEBUG_ACTION_CLEAR:
            sprintf(act, "SCHEDULER_DEBUG_ACTION_CLEAR");
            break;

        default:
            sprintf(act, "FBE_STATUS_GENERIC_FAILURE");
            break;

    }
    return act;
}

/*********************************************************************
* sepScheduler Class :: clear_all_debug_hooks(int argc, char ** argv)
*********************************************************************
*
*  Description:
*      Clear all debug hook in the scheduler for given object id.
*
*  Input: Parameters
*      argc  - argument count
*      *argv - pointer to arguments
*
*  Returns:
*      status - FBE_STATUS_OK or one of the other status codes.
*               See fbe_types.h
*
*  History
*     09-Sep-2011 : initial version
*********************************************************************/
fbe_status_t sepScheduler::clear_all_debug_hooks(
    int argc, char ** argv)
{
    // Define specific usage string  
    usage_format =
        " Usage: %s \n"
        " For example: %s \n";
    
    // Check that all arguments have been entered
    status = call_preprocessing(
        "clearAllDebugHooks",
        "sepScheduler::clear_all_debug_hooks",
        "fbe_api_scheduler_clear_all_debug_hooks",
        usage_format,
        argc, 6);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    sprintf(params, " ");

    // Make api call:
    status =  fbe_api_scheduler_clear_all_debug_hooks(&hook);

    // Check status of call
    if (status == FBE_STATUS_OK)
    {
        sprintf(temp,"Successfully clear all debug hook.\n");
    } else{
        sprintf(temp,"Failed to clear all debug hook..\n");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);

    return status;
}


/*********************************************************************
*  sepScheduler::Sep_Scheduler_Intializing_variable (private method)
*********************************************************************/
void sepScheduler::Sep_Scheduler_Intializing_variable()
{
     monitor_state_status = FBE_STATUS_FAILED;
    monitor_substate_status = FBE_STATUS_FAILED;
    check_type_status = FBE_STATUS_FAILED;
    action_status = FBE_STATUS_FAILED;
    fbe_zero_memory(&hook,sizeof(hook));
    return;
}

/*********************************************************************
* sepScheduler Class ::sep_scheduler_get_monitor_state_by_string(char **argv)
*********************************************************************
*
*  Description:
*      Get the monitor state as a string.
*
*  Input: Parameters
*      *argv - pointer to arguments
*
*  Returns:
*      void
*
*  History
*     16 July 2012 : initial version
*********************************************************************/
void sepScheduler::sep_scheduler_get_monitor_state_by_string(char **argv)
{
    if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD") == 0) {
            hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_REBUILD ;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD") == 0) {
            hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_REBUILD;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_VERIFY;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_VERIFY") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_METADATA_VERIFY;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_MARK_NR;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_CLEAR_RL;
      }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_EVAL_RL;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE") == 0) {
            hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE;
        }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_JOIN;
        }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL;
        }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_START") == 0) {
            hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_START;
        }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_EXPIRE") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_ACTIVATE_EVAL_RL_TIMER_EXPIRE;
        }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_VERIFY;
        }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_FLUSH;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REBUILD") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_JOURNAL_REBUILD;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_JOIN") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_JOIN;
        }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ZERO;
        }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY;
        }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_QUIESCE") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_QUIESCE;
        }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VERIFY_INVALIDATE") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_VERIFY_INVALIDATE;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_BLOCK_OPERATION") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_BLOCK_OPERATION;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EOL") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EOL;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_DOWNLOAD_PERMISSION") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_DOWNLOAD_PERMISSION;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_IN;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_COPY;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_SWAP_OUT;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_BASE_CONFIG_CLUSTERED_ACTIVATE;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_PERSIST") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_BASE_CONFIG_NON_PAGED_PERSIST;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_LUN_INCOMPLETE_WRITE_VERIFY") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_LUN_INCOMPLETE_WRITE_VERIFY;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_LUN_ZERO") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_LUN_ZERO;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_LUN_CLEAN_DIRTY") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_LUN_CLEAN_DIRTY;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_RAID_GROUP_PENDING") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_RAID_GROUP_PENDING;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_FAILED;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ABORT_COPY;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_VIRTUAL_DRIVE_ACTIVATE;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_EVAL_METADATA_ELEMENT;
    }
    else if (strcmp (*argv, "SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_NON_PAGED_INIT") == 0) {
        hook.monitor_state = SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_NON_PAGED_INIT;
    }
    else{
        hook.monitor_state = SCHEDULER_MONITOR_STATE_INVALID;
    }
}

/*********************************************************************
* sepScheduler Class ::sep_scheduler_get_monitor_substate_by_string(char **argv)
*********************************************************************
*
*  Description:
*      Get the monitor substate as string.
*
*  Input: Parameters
*      *argv - pointer to arguments
*
*  Returns:
*      void
*
*  History
*     16 July 2012 : initial version
*********************************************************************/
void sepScheduler::sep_scheduler_get_monitor_substate_by_string(char **argv)
{
    if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_ENTRY;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_REBUILD_SINGLY_DEGRADED") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_SINGLY_DEGRADED;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_DOWNSTREAM_PERMISSION") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_DOWNSTREAM_PERMISSION;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_UPSTREAM_PERMISSION") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_UPSTREAM_PERMISSION;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_PERMISSION") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_NO_PERMISSION;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_REBUILD_SEND") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_SEND;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_REBUILD_COMPLETED") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_COMPLETED;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_JOIN_REQUEST") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_JOIN_REQUEST;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_JOIN_STARTED") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_JOIN_STARTED;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_JOIN_DONE") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_JOIN_DONE;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_JOIN_SYNCED") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_JOIN_SYNCED;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_JOIN_REQUEST") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_JOIN_REQUEST;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_JOIN_STARTED") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_JOIN_STARTED;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_JOIN_DONE") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_JOIN_DONE;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_JOIN_SYNCED") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_JOIN_SYNCED;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ZERO_ENTRY;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ZERO_COMPLETION ;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_QUIESCE_ENTRY") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_QUIESCE_ENTRY;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_VERIFY_INVALIDATE_CHECKPOINT") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_VERIFY_INVALIDATE_CHECKPOINT;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_REMAP_EVENT") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_REMAP_EVENT;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_BLOCK_OPERATION_CHECK") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_BLOCK_OPERATION_CHECK;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_NON_MCR_REGION_CHECKPOINT") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_NON_MCR_REGION_CHECKPOINT;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_SET_EOL") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_SET_EOL;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_CLEAR_EOL") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_CLEAR_EOL;
    }
    else if (strcmp (*argv, "FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED") == 0) {
            hook.monitor_substate = FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_CONNECTED;
    }
    else if (strcmp (*argv, "FBE_VIRTUAL_DRIVE_SUBSTATE_MIRROR_MODE_SET") == 0) {
            hook.monitor_substate = FBE_VIRTUAL_DRIVE_SUBSTATE_MIRROR_MODE_SET;
    }
    else if (strcmp (*argv, "FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE") == 0) {
            hook.monitor_substate = FBE_VIRTUAL_DRIVE_SUBSTATE_COPY_COMPLETE;
    }
    else if (strcmp (*argv, "FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_DISCONNECTED") == 0) {
            hook.monitor_substate = FBE_VIRTUAL_DRIVE_SUBSTATE_EDGE_DISCONNECTED;
    }
    else if (strcmp (*argv, "FBE_VIRTUAL_DRIVE_SUBSTATE_PASS_THRU_MODE_SET") == 0) {
            hook.monitor_substate = FBE_VIRTUAL_DRIVE_SUBSTATE_PASS_THRU_MODE_SET;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_EVAL_RL_REQUEST;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED ;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_EVAL_RL_DONE;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED2") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED2;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_EVAL_RL_STARTED_BROKEN;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_REQUEST;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_DONE") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_DONE;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED2") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_CLEAR_RL_STARTED2 ;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_REQUEST") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_REQUEST;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_STARTED") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_STARTED;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_EVAL_MARK_NR_DONE;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_GOING_BROKEN") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_GOING_BROKEN;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_METADATA_MARK_RECONSTRUCT") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_METADATA_MARK_RECONSTRUCT;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_STARTED") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_STARTED;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_ERROR") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_JOURNAL_VERIFY_ERROR;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_VERIFY_CHECKPOINT;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_START_METADATA_VERIFY") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_START_METADATA_VERIFY ;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_INCOMPLETE_WRITE") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_INCOMPLETE_WRITE;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_NEEDS_REBUILD") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_MARK_METADATA_NEEDS_REBUILD ;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_VERIFY_NO_PERMISSION") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_VERIFY_NO_PERMISSION;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_VERIFY_SEND") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_VERIFY_SEND;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RO_START") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RO_START;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RW_START") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_VERIFY_USER_RW_START;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_VERIFY_INCOMPLETE_WRITE_START") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_VERIFY_INCOMPLETE_WRITE_START;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_VERIFY_SYSTEM_START") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_VERIFY_SYSTEM_START;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_VERIFY_ERROR_START;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_VERIFY_COMPLETED;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_VERIFY_ALL_VERIFIES_COMPLETE") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_VERIFY_ALL_VERIFIES_COMPLETE;
    }
    else if (strcmp (*argv, "FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_REQUEST") == 0) {
            hook.monitor_substate = FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_REQUEST;
    }
    else if (strcmp (*argv, "FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_STARTED") == 0) {
            hook.monitor_substate = FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_STARTED;
    }
    else if (strcmp (*argv, "FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_CLEARED") == 0) {
            hook.monitor_substate = FBE_BASE_CONFIG_SUBSTATE_CLUSTERED_ACTIVATE_CLEARED;
    }
    else if (strcmp (*argv, "FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_PERSIST") == 0) {
            hook.monitor_substate = FBE_BASE_CONFIG_SUBSTATE_NON_PAGED_PERSIST;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_QUEUED") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_QUEUED;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_COMPLETED") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_COMPLETED;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_NO_PERMISSION") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_NO_PERMISSION;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_SEND_VERIFY") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_SEND_VERIFY;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ZERO_SEND;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_ZERO_NO_PERMISSION") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ZERO_NO_PERMISSION;
    }
    else if (strcmp (*argv, "FBE_LUN_SUBSTATE_INCOMPLETE_WRITE_VERIFY_REQUESTED") == 0) {
            hook.monitor_substate = FBE_LUN_SUBSTATE_INCOMPLETE_WRITE_VERIFY_REQUESTED;
    }
    else if (strcmp (*argv, "FBE_LUN_SUBSTATE_ZERO_CHECKPOINT_UPDATED") == 0) {
            hook.monitor_substate = FBE_LUN_SUBSTATE_ZERO_CHECKPOINT_UPDATED  ;
    }
    else if (strcmp (*argv, "FBE_LUN_SUBSTATE_LAZY_CLEAN_START") == 0) {
            hook.monitor_substate = FBE_LUN_SUBSTATE_LAZY_CLEAN_START  ;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_START") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_START;
    }
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_DONE") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_JOURNAL_FLUSH_DONE;
     } 
    else if (strcmp (*argv, "FBE_RAID_GROUP_SUBSTATE_REBUILD_DOWNSTREAM_HIGHER_PRIORITY") == 0) {
            hook.monitor_substate = FBE_RAID_GROUP_SUBSTATE_REBUILD_DOWNSTREAM_HIGHER_PRIORITY;
     } 
    else if (strcmp (*argv, "FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED") == 0) {
            hook.monitor_substate = FBE_VIRTUAL_DRIVE_SUBSTATE_DRIVE_REMOVED;
     } 
    else if (strcmp (*argv, "FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_DENIED") == 0) {
            hook.monitor_substate = FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_DENIED;
     } 
    else if (strcmp (*argv, "FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_GRANTED") == 0) {
            hook.monitor_substate = FBE_VIRTUAL_DRIVE_SUBSTATE_ABORT_COPY_GRANTED;
     } 
    else if (strcmp (*argv, "FBE_VIRTUAL_DRIVE_SUBSTATE_SET_PASS_THRU_CONDITION") == 0) {
            hook.monitor_substate = FBE_VIRTUAL_DRIVE_SUBSTATE_SET_PASS_THRU_CONDITION;
     } 
    else if (strcmp (*argv, "FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE") == 0) {
            hook.monitor_substate = FBE_VIRTUAL_DRIVE_SUBSTATE_INITIATE_PASS_THRU_MODE;
     } 
    else if (strcmp (*argv, "FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE") == 0) {
            hook.monitor_substate = FBE_VIRTUAL_DRIVE_SUBSTATE_SWAP_OUT_COMPLETE;
     } 
    else if (strcmp (*argv, "FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY") == 0) {
            hook.monitor_substate = FBE_VIRTUAL_DRIVE_SUBSTATE_HEALTHY;
     } 
    else if (strcmp (*argv, "FBE_VIRTUAL_DRIVE_SUBSTATE_BROKEN") == 0) {
            hook.monitor_substate = FBE_VIRTUAL_DRIVE_SUBSTATE_BROKEN;
     } 
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_FIRST_CREATE") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_FIRST_CREATE;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_NOT_FIRST_CREATE") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_EVAL_METADATA_ELEM_NOT_FIRST_CREATE;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_NONPAGED_INIT") == 0) {
            hook.monitor_substate=FBE_PROVISION_DRIVE_SUBSTATE_NONPAGED_INIT;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_DOWNLOAD_PERMISSION") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_DOWNLOAD_PERMISSION;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_START;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_REQUEST") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_REQUEST;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_SUCCESS") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_SUCCESS;
    }
    else if (strcmp (*argv, "FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_COMPLETE") == 0) {
            hook.monitor_substate = FBE_PROVISION_DRIVE_SUBSTATE_PAGED_RECONSTRUCT_COMPLETE;
    }
    else{
            hook.monitor_substate = FBE_SCHEDULER_MONITOR_SUB_STATE_INVALID;
        }

    }

/*********************************************************************
* sepScheduler Class ::sep_scheduler_get_check_type_as_string(char** argv)
*********************************************************************
*
*  Description:
*      Get the check type as a string.
*
*  Input: Parameters
*      *argv - pointer to arguments
*
*  Returns:
*      void
*
*  History
*     16 July 2012 : initial version
*********************************************************************/
void sepScheduler::sep_scheduler_get_check_type_as_string(char** argv)
{
    if (strcmp (*argv, "SCHEDULER_CHECK_STATES") == 0) {
        hook.check_type = SCHEDULER_CHECK_STATES;
    }
    else if (strcmp (*argv, "SCHEDULER_CHECK_VALS_EQUAL") == 0) {
        hook.check_type = SCHEDULER_CHECK_VALS_EQUAL;
    }
    else if (strcmp (*argv, "SCHEDULER_CHECK_VALS_LT") == 0) {
        hook.check_type = SCHEDULER_CHECK_VALS_LT;
    }
    else if (strcmp (*argv, "SCHEDULER_CHECK_VALS_GT") == 0) {
        hook.check_type =     SCHEDULER_CHECK_VALS_GT;
    }
    else{
        hook.check_type = FBE_STATUS_GENERIC_FAILURE;
    }
}

/*********************************************************************
* sepScheduler Class ::sep_scheduler_get_action_as_string(char **argv)
*********************************************************************
*
*  Description:
*      get the action.
*
*  Input: Parameters
*      *argv - pointer to arguments
*
*  Returns:
*      void
*
*  History
*     16 July 2012 : initial version
*********************************************************************/

void sepScheduler:: sep_scheduler_get_action_as_string(char **argv)
{
    if (strcmp (*argv, "SCHEDULER_DEBUG_ACTION_LOG") == 0) {
        hook.action = SCHEDULER_DEBUG_ACTION_LOG;
    }
    else if (strcmp (*argv, "SCHEDULER_DEBUG_ACTION_CONTINUE") == 0) {
        hook.action = SCHEDULER_DEBUG_ACTION_CONTINUE;
    }
    else if (strcmp (*argv, "SCHEDULER_DEBUG_ACTION_PAUSE") == 0) {
        hook.action = SCHEDULER_DEBUG_ACTION_PAUSE;
    }
    else if (strcmp (*argv, "SCHEDULER_DEBUG_ACTION_CLEAR") == 0) {
        hook.action = SCHEDULER_DEBUG_ACTION_CLEAR;
    }
    else{
        hook.action = FBE_STATUS_GENERIC_FAILURE;
    }
}

