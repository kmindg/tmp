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
*      This file defines the methods of the SEP LUN INTERFACE class.
*
*  Notes:
*      The SYS EVENT LOG class (sysEventLog) is a derived class of 
*      the base class (bSYS).
*
*  History:
*      04-May-2011 : inital version
*
*********************************************************************/

#ifndef T_SYS_EVENT_LOG_CLASS_H
#include "sys_event_log_class.h"
#endif

/*********************************************************************
* sysEventLog class :: Constructor
*********************************************************************/

sysEventLog::sysEventLog() : bSYS()
{
    // Store Object number
    idnumber = (unsigned) SYS_EVENT_LOG_SERVICE_INTERFACE;
    sysEventLogCount = ++gSysEventLogCount;

    // Store Object name
    idname = new char [64];
    strcpy(idname, "SYS_EVENT_LOG_SERVICE_INTERFACE");

    Sys_Log_Intializing_variable();

    if (Debug) {
        sprintf(temp, "sysEventLog class constructor (%d) %s\n", 
                idnumber, idname);
        vWriteFile(dkey, temp);
    }

    // <List of FBE API SYS EVENT LOG Interface function calls>
    sysEventLogInterfaceFuncs = 
        "[function call]\n" \
        "[short name] [FBE API SYS EVENT LOG SERVICE Interface]\n" \
        " ------------------------------------------------------------\n" \
        " clearLog        fbe_api_clear_event_log\n" \
        " getLog          fbe_api_get_event_log\n" \
        " msgCount        fbe_api_get_event_log_msg_count\n" \
        " getStats        fbe_api_get_event_log_statistics\n"\
        " clearStats      fbe_api_clear_event_log_statistics\n"\
        " ------------------------------------------------------------\n"\
        " getFailureInfo   fbe_api_system_get_failure_info\n"\
        " ------------------------------------------------------------\n";

    // Define common usage for SYS Event Log commands
    usage_format =
        " Usage: %s [Package] \n"
        " For example: %s <PHY|SEP|ESP>";
};

/*********************************************************************
* sysEventLog class : Accessor methods
*********************************************************************/

unsigned sysEventLog::MyIdNumIs(void)
{
    return idnumber;
};

char * sysEventLog::MyIdNameIs(void)
{
    return idname;
};

void sysEventLog::dumpme(void)
{ 
    strcpy (key, "sysEventLog::dumpme");
    sprintf(temp, "Object id: (%d) count: (%d)\n", 
            idnumber, sysEventLogCount);
    vWriteFile(key, temp);
}

/*********************************************************************
* sysEventLog Class :: select()
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
*      2011-04-05 : inital version
*
*********************************************************************/

fbe_status_t sysEventLog::select(int index, int argc, char *argv[]) 
{
    // Assign default values
    status = FBE_STATUS_GENERIC_FAILURE;
    strcpy (key, "Select SYS interface"); 
    c = index;

    // List SEP LUN Interface calls if help option and less than 6
    // arguments were entered.
    if (help && argc <= 6) {
        strcpy (key, "Argument [short name]");
        HelpCmds( (char *) sysEventLogInterfaceFuncs );

        // clear log
    } else if (strcmp (argv[index], "CLEARLOG") == 0) {
        status = clear_log(argc, &argv[index]);
        // get log
    } else if (strcmp (argv[index], "GETLOG") == 0) {
        status = get_log(argc, &argv[index]);
        // get msg count
    } else if (strcmp (argv[index], "MSGCOUNT") == 0) {
        status = get_msg_count(argc, &argv[index]);
        // get event log statstics
    } else if (strcmp (argv[index], "GETSTATS") == 0) {
        status = get_log_stats(argc, &argv[index]);
        // get event log statstics
    } else if (strcmp (argv[index], "CLEARSTATS") == 0) {
        status = clear_stats(argc, &argv[index]);
        // get failure info method
    } else if (strcmp (argv[index], "GETFAILUREINFO") == 0) {
        status = get_failure_info(argc, &argv[index]);

        // can't find match on short name
    } else {
        sprintf(temp, "<ERROR!> Argument [short name] %s [%s]\n", 
                BAD_INPUT, argv[index]);
        vWriteFile(ukey, temp);
        HelpCmds( (char *) sysEventLogInterfaceFuncs );
    }

    return status;
}

/*********************************************************************
* sysEventLog class :: clear_log () 
*********************************************************************/

fbe_status_t sysEventLog::clear_log(int argc , char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "clearLog",
        "sysEventLog::clear_log",
        "fbe_api_clear_event_log",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    // Validate the package
    package_id = utilityClass::convert_string_to_package_id(*argv);
    sprintf(params, " package id 0x%x (%s)", package_id, *argv);
    if (package_id == FBE_PACKAGE_ID_INVALID) {
        sprintf(temp, " Invalid package %s. Valid packages are PHY, SEP, or ESP.\n",*argv);
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call
    status = fbe_api_clear_event_log(package_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't clear event log");
    } else {
        sprintf(temp, "Event log cleared");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sysEventLog class :: get_log () 
*********************************************************************/

fbe_status_t sysEventLog::get_log(int argc , char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getLog",
        "sysEventLog::get_log",
        "fbe_api_get_event_log",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    // Validate the package
    package_id = utilityClass::convert_string_to_package_id(*argv);
    sprintf(params, " package id 0x%x (%s)", package_id, *argv);
    if (package_id == FBE_PACKAGE_ID_INVALID) {
        sprintf(temp, " Invalid package %s. Valid packages are PHY, SEP, or ESP.\n",*argv);
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call
    status = fbe_api_get_event_log_statistics(&event_log_statistics,
                                              package_id);

    char* data;
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "\nFailed to get event log statistics \n");
        data = temp;
    } else if (event_log_statistics.total_msgs_logged == 0) {
        sprintf(temp, "\n No message logged for this package \n");
        data = temp;
    } else {
        // Get the log entries
        data = construct_log();
    }

    // Output results from call or description of error
    call_post_processing(status,data, key, params);

    return status;
}

/*********************************************************************
* sysEventLog class :: get_msg_count () 
*********************************************************************/

fbe_status_t sysEventLog::get_msg_count(int argc , char ** argv) {

    // Define specific usage string  
    usage_format =
        "Usage: %s [Package] [MessageId]\n"
        "For example: %s <PHY|SEP|ESP> 0xe167800a";

    // Check that all arguments have been entered
    status = call_preprocessing(
        "msgCount",
        "sysEventLog::get_msg_count",
        "fbe_api_get_event_log_msg_count",
        usage_format,
        argc, 8);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    // Validate the package
    package_id = utilityClass::convert_string_to_package_id(*argv);
    if (package_id == FBE_PACKAGE_ID_INVALID) {
        sprintf(temp, " Invalid package %s. Valid packages are PHY, SEP, or ESP.\n",*argv);
        sprintf(params, " package id 0x%x (%s)", package_id, *argv);
        call_post_processing(status, temp, key, params);
        return status;
    }

    argv++;
    // Convert message id to hex
    msg_id  = (fbe_u32_t)strtoul(*argv, 0, 0);

    argv--;
    sprintf(params, " package id 0x%x (%s) message id (0x%x)",
            package_id, *argv, msg_id);

    event_log_msg.msg_id = msg_id;
    // Make api call
    status = fbe_api_get_event_log_msg_count(&event_log_msg, package_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't get event log message count.");
    } else {
        sprintf(temp, "Message Count is: (%d).", event_log_msg.msg_count);
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;

}

/*********************************************************************
* sysEventLog class :: get_log_stats () 
*********************************************************************/

fbe_status_t sysEventLog::get_log_stats(int argc , char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "getStats",
        "sysEventLog::get_log_stats",
        "fbe_api_get_event_log_statistics",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    // Validate the package
    package_id = utilityClass::convert_string_to_package_id(*argv);
    sprintf(params, " package id 0x%x (%s)", package_id, *argv);
    if (package_id == FBE_PACKAGE_ID_INVALID) {
        sprintf(temp, " Invalid package %s. Valid packages are PHY, SEP, or ESP.\n",*argv);
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call
    status = fbe_api_get_event_log_statistics(&event_log_statistics,
                                              package_id);
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "\nFailed to get event log statistics \n");
    } else {
        // Format statistics output
        sprintf(temp,              "\n "
            "<Total Messages Logged>           %u\n "
            "<Total Messages Queued>           %u\n "
            "<Total Messages Dropped>          %u\n "
            "<Current Messages Logged>         %u\n ",
            event_log_statistics.total_msgs_logged,
            event_log_statistics.total_msgs_queued,
            event_log_statistics.total_msgs_dropped,
            event_log_statistics.current_msgs_logged);
    }

    // Output results from call or description of error
    call_post_processing(status,temp, key, params);

    return status;
}

/*********************************************************************
* sysEventLog class :: clear_stats () 
*********************************************************************/

fbe_status_t sysEventLog::clear_stats(int argc , char ** argv)
{
    // Check that all arguments have been entered
    status = call_preprocessing(
        "clearStats",
        "sysEventLog::clear_stats",
        "fbe_api_clear_event_log_statistics",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    // Validate the package
    package_id = utilityClass::convert_string_to_package_id(*argv);
    sprintf(params, " package id 0x%x (%s)", package_id, *argv);
    if (package_id == FBE_PACKAGE_ID_INVALID) {
        sprintf(temp, " Invalid package %s. Valid packages are PHY, SEP, or ESP.\n",*argv);
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call: 
    status = fbe_api_clear_event_log_statistics(package_id);

    // Check status of call
    if (status != FBE_STATUS_OK) {
        sprintf(temp, "Can't clear event log statistics");
    } else {
        sprintf(temp, "Event log statistics cleared");
    }

    // Output results from call or description of error
    call_post_processing(status, temp, key, params);
    return status;
}

/*********************************************************************
* sysEventLog class :: construct_log () 
*********************************************************************/
char* sysEventLog::construct_log() {

    char *data = NULL;
    char *mytemp = NULL;
    fbe_u32_t total_events_copied = 0;
    fbe_u32_t loop_count = 0;

    event_log_info = (fbe_event_log_get_event_log_info_t* )fbe_api_allocate_memory(sizeof(fbe_event_log_get_event_log_info_t) * FBE_EVENT_LOG_MESSAGE_COUNT);
    status = fbe_api_get_all_events_logs(&event_log_info[loop_count],&total_events_copied, FBE_EVENT_LOG_MESSAGE_COUNT,
                                                                 package_id);
    if (status != FBE_STATUS_OK)
    {
        sprintf(mytemp, "\nFailed to get event log info \n");
        strcpy(data, mytemp);
        delete [] mytemp;
        return data;
    }
    if(total_events_copied == 0){
        sprintf(temp, "\n No message logged for this package \n");
        data = temp;
        return data;
    }

    // loop through the event log messages
    for(loop_count =0;loop_count<total_events_copied;loop_count++) {

        mytemp = new char[1024];

        // Get the event log data in the right format
        sprintf(mytemp, "%02d:%02d:%02d  0x%x  %s\n",
                       event_log_info[loop_count].event_log_info.time_stamp.hour,
                       event_log_info[loop_count].event_log_info.time_stamp.minute,
                       event_log_info[loop_count].event_log_info.time_stamp.second,
                       event_log_info[loop_count].event_log_info.msg_id,
                       event_log_info[loop_count].event_log_info.msg_string);
        
        // Append the event to the lis
        data = utilityClass::append_to(data, mytemp);

        delete [] mytemp;
    }

    return data;
}

/*********************************************************************
* sysEventLog::Sys_Log_Intializing_variable (private method)
*********************************************************************/
void sysEventLog::Sys_Log_Intializing_variable()
{
    msg_id = 0;
    fbe_zero_memory(&event_log_statistics,sizeof(event_log_statistics));
    fbe_zero_memory(&event_log_msg,sizeof(event_log_msg));
    fbe_zero_memory(&err_info_ptr,sizeof(err_info_ptr));
}

/*********************************************************************
* sysEventLog class :: get_failure_info () 
*********************************************************************/
fbe_status_t sysEventLog::get_failure_info(int argc , char ** argv)
{

    // Check that all arguments have been entered
    status = call_preprocessing(
        "getfailureinfo",
        "sysEventLog::get_failure_info",
        "fbe_api_system_get_failure_info",
        usage_format,
        argc, 7);

    // Return if error found or help option entered
    if (status != FBE_STATUS_OK) {
        return status;
    }

    argv++;
    // Validate the package
    package_id = utilityClass::convert_string_to_package_id(*argv);
    sprintf(params, " package id 0x%x (%s)", package_id, *argv);
    if (package_id == FBE_PACKAGE_ID_INVALID) {
        sprintf(temp, " Invalid package %s. Valid packages are PHY, SEP, or ESP.\n",*argv);
        call_post_processing(status, temp, key, params);
        return status;
    }

    // Make api call
    status = fbe_api_system_get_failure_info(&err_info_ptr);

    if (status != FBE_STATUS_OK) {
        sprintf(temp, "\nFailed to get system failure info \n");
    } else {
        sprintf(temp, "\n<Trace critical error counter>   0x%x\n"
                      "<Trace error counter>            0x%x\n", 
        err_info_ptr.error_counters[package_id].trace_critical_error_counter,
        err_info_ptr.error_counters[package_id].trace_error_counter);
    }

    // Output results from call or description of error
    call_post_processing(status,temp, key, params);

    return status;
}

/*********************************************************************
* sysEventLog end of file
*********************************************************************/
