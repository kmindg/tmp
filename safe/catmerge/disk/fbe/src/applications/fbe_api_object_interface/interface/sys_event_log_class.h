/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2009
 * All Rights Reserved.
 * Licensed Material-Property of EMC Corporation.
 * This software is made available solely pursuant to the terms
 * of a EMC license agreement which governs its use.
 *********************************************************************/

/*********************************************************************
 *
 *  Description:
 *      This file defines the SYS EVENT LOG INTERFACE class.
 *
 *  History:
 *      2011-04-05 : inital version
 *
 *********************************************************************/

#ifndef T_SYS_EVENT_LOG_CLASS_H
#define T_SYS_EVENT_LOG_CLASS_H

#ifndef T_SYS_CLASS_H
#include "sys_class.h"
#endif

/*********************************************************************
 *          SYS Event Log class : bSYS base class
 *********************************************************************/

class sysEventLog: public bSYS
{
    protected:

        // Every object has an idnumber
        unsigned idnumber;
        unsigned sysEventLogCount;

        // interface object name
        char  * idname;

        // SYS EVENTLOG Interface function calls and usage
        char * sysEventLogInterfaceFuncs;
        char * usage_format;

        // SYS EVENTLOG interface fbe api data structures
        fbe_event_log_get_event_log_info_t *event_log_info;
        fbe_event_log_statistics_t event_log_statistics;
        fbe_event_log_msg_count_t event_log_msg;
        fbe_u32_t msg_id;
        fbe_api_system_get_failure_info_t err_info_ptr;

        // private methods 
        char* construct_log();
        bool is_valid_package(char ** argv);
        void Sys_Log_Intializing_variable();

    public:

        // Constructor/Destructor
        sysEventLog();
        ~sysEventLog(){}

        // Accessor methods
        char   * MyIdNameIs(void);
        unsigned MyIdNumIs(void);
        int      MyCountIs(void);
        void     dumpme(void);

        // Select method to call FBE API
        fbe_status_t select(int index, int argc, char *argv[]);

        // FBE API clear log method
        fbe_status_t clear_log (int argc, char ** argv);

        // FBE API get log method
        fbe_status_t get_log (int argc, char ** argv);

        // FBE API get message count method
        fbe_status_t get_msg_count(int argc, char ** argv);

        // FBE API get log stats method
        fbe_status_t get_log_stats(int argc, char ** argv);

        // FBE API clear stats method
        fbe_status_t clear_stats(int argc, char ** argv);

        // FBE API get failure info method
        fbe_status_t get_failure_info(int argc , char ** argv);

        // ------------------------------------------------------------
};

#endif
