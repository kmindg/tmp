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
 *      This file defines the SEP SCHEDULER INTERFACE class.
 *
 *  History:
 *      9-Sep-2011 : inital version
 *
 *********************************************************************/

#ifndef T_SEP_SCHEDULER_CLASS_H
#define T_SEP_SCHEDULER_CLASS_H

#ifndef T_SEP_CLASS_H
#include "sep_class.h"
#endif

/*********************************************************************
 *          SEP SCHEDULER class : bSEP base class
 *********************************************************************/

class sepScheduler : public bSEP 
{
     protected:
        // Every object has an idnumber
        unsigned idnumber;
        unsigned sepSchedulerCount;

        // interface object name
        char  * idname;

        // SEP Scheduler Interface function calls and usage
        char * sepSchedulerInterfaceFuncs;
        char * usage_format ;

        // SEP Scheduler interface fbe api data structures
        fbe_scheduler_debug_hook_t hook;
        fbe_status_t monitor_state_status;
        fbe_status_t monitor_substate_status;
        fbe_status_t check_type_status;
        fbe_status_t action_status;
        char mon_state[60], mon_substate[60],act[60], check[60];
        fbe_u32_t monitor_state, monitor_substate;

        //Private methods
        fbe_status_t  extract_scheduler_debug_hook_args(char**  argv);
        fbe_status_t  verify_monitor_state( fbe_u32_t monitor_state);
        fbe_status_t  verify_monitor_substate( fbe_u32_t monitor_substate);
        fbe_status_t verify_check_type(fbe_u32_t check_type);
        fbe_status_t  verify_action(fbe_u32_t action);
        char* edit_monitor_state(fbe_u32_t monitor_state);
        char* edit_monitor_substate(fbe_u32_t monitor_substate);
        char* edit_check_type(fbe_u32_t check_type);
        char* edit_action(fbe_u32_t action);
        void Sep_Scheduler_Intializing_variable();
        void sep_scheduler_get_monitor_state_by_string(char **argv);
        void sep_scheduler_get_monitor_substate_by_string(char **argv);
        void sep_scheduler_get_check_type_as_string(char** argv);
        void  sep_scheduler_get_action_as_string(char **argv);
        
     public:

        // Constructor/Destructor
        sepScheduler();
        ~sepScheduler(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // FBE API Scheduler methods
        fbe_status_t add_debug_hook(int argc,  char ** argv);
        fbe_status_t delete_debug_hook(int argc,  char ** argv);
        fbe_status_t get_debug_hook(int argc,  char ** argv);
        fbe_status_t clear_all_debug_hooks(int argc, char ** argv);
};

#endif
