/***************************************************************************
 * Copyright (C) EMC Corporation 2007 - 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_control.cpp
 ***************************************************************************
 *
 * DESCRIPTION: MUT Framework
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    09/18/2007   Buckley, Martin   initial version
 *    10/17/2007   Alexander Gorlov  and Kirill Timofeev global rework
 *    10/25/2007   Kirill Timofeev   added support for multiple testsuites
 *    10/30/2007   Igor Bobrovnikov  multi-choice prompt for ASKUSER abort policy
 *    02/29/2008   Igor Bobrovnikov  new log format
 *    10/06/2010   JD C++ rewrite
 *                 Nachiket Londhe   Linux rewrite
 **************************************************************************/

#include <stdlib.h>
#include <stdio.h>
//#include <sys/types.h>
//#include <time.h>
#include <string.h>
//#include <ctype.h>
#include <vector>

/*
 * This code is being commented out for time being till 
 * we figure out how to compile code without adding windows.h
 */
 
//#include <windows.h>

using namespace std;

#include "mut_sdk.h"
#include "mut/defaulttest.h"
#include "mut_test_control.h"
#include "mut_log.h"
#include "mut_options.h"

#include "mut_config_manager.h"
#include "EmcPAL_Misc.h"


/**
 * In the process of second checkin to windows, all the following log4cxx code
 * will be disabled, as we don't have the clearance for using log4cxx library
 *
 * proxy logger declared below will just print out the basic messaged to console
 * and to the file mut_log.log
 */

//#include "log4cxx/logger.h"
//
//using namespace log4cxx;

Mut_test_control *mut_control;

//local static functions

static int mut_run_suite_once(DefaultSuite *suite,int index);
static int mut_exit_monitor_entered = 0;

/** \fn void mut_process_test_failure(void)
 *  \details analyzing of current abort policy and perform
 *           appropriate action
 */
void mut_process_test_failure(void){

    //Register the test failure
    mut_control->current_test_runner->setTestStatus(MUT_TEST_FAILED);
}

static bool _mut_init_called = false; /* TRUE when mut_init() has been already called */

/* doxygen documentation for this function can be found in mut.h */
void  mut_init(int argc, char **argv)
{
    Mut_config_manager *mcm;
    
    /*
     * This code is being commented out for time being till 
     * we figure out how to compile code without adding windows.h
     *   
    char computerNameHeader[128];
    char computerName[64];
    char userNameHeader[128];
    char userName[64];
     */
    
    DWORD length = 64;
    
    if(_mut_init_called)
    {
        printf("[MUT user program error: mut_init() has been called more than once]");
        exit(1);
    }

    _mut_init_called = true;

    mut_control = new Mut_test_control(new Mut_log()); //this pointer is globally available

    mcm = new Mut_config_manager(argc,argv);// option parsing done here, note also the config manager goes out
                                            // of scope and is not available once mut_init exits.

    // See if we can get the Process Affinity here and the numa information.
    // Get number of cpus being used so we can set up correct delays
    if(csx_p_phys_topology_info_query(&mut_control->mPhys_topology) != CSX_STATUS_SUCCESS) {
        printf("[MUT user program error: mut_init() cannot determine test topology]");
        // Trap into debugger so it is easy to find out where the extra call made
        EmcpalDebugBreakPoint(); // In autotestmode, this will generate an exception, coredump and exit 
        exit(1); /* The program shall not be executed longer as its source is not correct */
    }
    CSX_BREAK();

    mut_control->set_start(mcm->get_start());

    mut_control->set_end(mcm->get_end());
    
    delete mcm; 
    
    
    /*
     * This code is being commented out for time being till 
     * we figure out how to compile code without adding windows.h
     *
     
    
    //Get computer name - windows environemnt only
    GetComputerName(computerName,&length);
    
    //Get user name - windows environemnt only
    GetUserName(userName, &length);
    
    sprintf(computerNameHeader,"ComputerName %s \n",computerName);
    
    sprintf(userNameHeader,"UserName %s \n",userName);
    
    mut_control->mut_log->log_message(computerNameHeader);
    mut_control->mut_log->log_message(userNameHeader);

    */
    
    return;
}

void set_global_timeout(unsigned long timeout){
    mut_control->timeout.set(timeout);
}

char *mut_get_user_option_value(const char *option_name)
{
    char *value;
    Option *option = Options::get_option(option_name);
    if(option == NULL) {
        value = NULL;
    }
    else {
        String_option *stringOpt = static_cast<String_option *>(option);
        value = stringOpt->get();
        if(value != NULL && *value == '\0') {
            value = NULL;
        }
    }

    return value;
}

int  mut_option_selected(const char *option_name) {
    Option *option = Options::get_option(option_name);

    return (option == NULL) ? 0 : option->isSet();
}


int mut_register_user_option(const char *option_name, int least_num_of_args, int show, const char *description){ //why does this return anything??
    // argc supplied by user is one less than what option system expects.  add 1
    new mut_user_option(option_name, least_num_of_args+1, (show ? true:false), description); // convert show from BOOL(int) to bool
    return 1;
}

