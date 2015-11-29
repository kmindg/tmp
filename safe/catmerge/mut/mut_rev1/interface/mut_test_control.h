/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_test_control.h
 ***************************************************************************
 *
 * DESCRIPTION: This class holds all the control data for mut
 *      It's treated like a singleton, initialized once in mut_control and
 *      generally referenced as needed
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/2010 Created JD
 **************************************************************************/


#ifndef MUT_TEST_CONTROL_H
#define MUT_TEST_CONTROL_H
#include "generic_types.h"
#include <vector>
#include <string>
#include "EmcPAL.h"
#include "EmcPAL_DriverShell.h"
#include "EmcPAL_Misc.h"
#include "mut_testsuite.h"
#include "mut_test_entry.h"
#include "mut_private.h"
#include "mut_sdk.h"
#include "mut_log.h"

#include "mut_config_manager.h"
#include "mut_abstract_test_runner.h"
#include "simulation/Segment_option.h"
#include "simulation/RBA_option.h"
#include "simulation/PlatformConfig_option.h"
#include "simulation/CacheSize_option.h"
#include "simulation/Multiple_Memory_Segment_Opt.h"
#include "simulation/VirtualAffinity_option.h"
#include "simulation/SplitAffinity_option.h"
#include "simulation/debug_option.h"

#define ITERATION_ENDLESS 0

class MutString {
public:
    const char * c_str() {return _string.c_str();}
    MutString(const char *s):_string(s){};
    ~MutString() {};
    
private:
    string _string;
};

class ListOfSuites {
public:
    ListOfSuites() {};
    ~ListOfSuites() {};

    vector<Mut_testsuite *>::iterator get_list_begin() { return list.begin(); }
    vector<Mut_testsuite *>::iterator get_list_end()   { return list.end(); }
    void push_back(Mut_testsuite *v) { try {list.push_back(v);} catch(...){MUT_INTERNAL_ERROR(("ListOfSuites push_back exception"));} }
    void clear() { list.clear(); }
    UINT_32 size() { return (UINT_32)list.size(); }
    bool empty() { return list.empty(); }
    Mut_testsuite * back() {return list.back();}
    

private:
    vector<Mut_testsuite *> list;
};

class ListOfTests {
public:
    ListOfTests() {};
    ~ListOfTests() {};

    vector<Mut_test_entry *>::iterator get_list_begin() { return list.begin(); }
    vector<Mut_test_entry *>::iterator get_list_end()   { return list.end(); }
    void push_back(Mut_test_entry *v) { try {list.push_back(v);} catch(...){MUT_INTERNAL_ERROR(("ListOfTests push_back exception"));} }
    void clear() { list.clear(); }
    UINT_32 size() { return (UINT_32)list.size(); }
    bool empty() { return list.empty(); }
    Mut_test_entry * back() {return list.back();}

private:
    vector<Mut_test_entry *> list;
};


/** \class mut_test_control
 *  \details
 *  This structure holds all settings and pointers for running tests. It
 *  exists as single static instance and is initialized by mut_init function.
 */
class Mut_test_control {
private:
    //mut_testsuite_start_callback_func_t suite_start_callback_func;
    int start;
    int end;
        

public:
// option variables
    mut_list_option list_mode;
    mut_info_option info_mode;
    mut_isolate_option isolate_mode;
    mut_isolated_option isolated_mode;
    mut_run_tests_option run_tests;
    mut_run_testsuites_option run_testsuites;
    mut_timeout_option timeout;
    mut_iteration_option iteration;
    mut_monitorexit_option monitorexit;
    mut_core_option core;
    mut_nodebugger_option nodebugger;
    mut_nofailontimeout_option no_fail_on_timeout;
    sp_todebug_option sptodebug;
    Cache_Size_option cache_size;
    Multiple_Memory_Segment_option mms;
    Split_Affinity_option sa;
    Platform_Config_option platform_config;
    RBA_option rba_tracing_enabled;
    mut_dump_on_timeout_option dump_on_timeout;
    mut_test_pause_option test_pause;
    mut_help_option help;
    csx_p_phys_topology_info_t *mPhys_topology;

    Mut_log *mut_log;
   
    EMCPAL_MUTEX mMutex; // Mutex will be used in the abort path    
    int mutExecutionStatus;
    
    mut_execution_t   get_mode();

    Mut_test_control(Mut_log *log);

    ~Mut_test_control();


    Mut_abstract_test_runner *current_test_runner;
    Mut_testsuite *current_suite; /**< Testsuite that is currently running */
    ListOfSuites *suite_list;     /**< Testsuites are stored in a vector.*/

    /** \fn void process_test_error(enum Mut_test_status_e status)
     *  \param    status -  status of test
     *  \details register test error
     */

    void process_test_error(enum Mut_test_status_e status);

    /** \fn Mut_testsuite *find_testsuite(char *name)
    *  \param name - symbolic name of the testsuite
    *  \details
    *  This function searches linked list of testsuites for testsuite with
    *  given name. If found returns pointer to this testsuite, NULL otherwise.
    */
    Mut_testsuite *find_testsuite(char *name);

    // start and end of test number range to run
    int get_start();
    int get_end();

    void set_start(int s);
    void set_end(int e);

    unsigned long get_default_timeout();

    Mut_abstract_test_runner *runfactory(Mut_testsuite *suite, Mut_test_entry *test);

    bool mut_is_test_pause() {
        return test_pause.get();
    }

    /** \fn void Add_UnhandledExceptionHandler() 
      *  \details
      *  Add unhandled exception handling support when autotestmode is set
    */
    void Add_UnhandledExceptionHandler();

    /** \fn LONG WINAPI void Remove_UnhandledExceptionHandler()
     *  \details
     *  Remove the unhandled exception handling support
     */

    void Remove_UnhandledExceptionHandler();
      
    // This method will be used to determine if a debugger should be opened or exception should be caught in the handler
    static csx_status_e mut_c_exception_filter(csx_rt_proc_exception_info_t *exception, csx_rt_proc_catch_filter_context_t context);

    /** \fn static csx_status_e UnhandledExceptionHandler(csx_rt_proc_exception_info_t *info)
     *  \param - *info pointer to the exception infomation
     *  \details
     *  This handler will trap all unhandled exceptions, including the ones in other context's. 
     *  By enabling this unhandled exception handler (-autoTestMode) we will catch all unhandled and will exit the application gracefully. 
     *  (i.e. ending the suite, closing all logs etc...).
     */
    static csx_status_e CSX_GX_RT_DEFCC UnhandledExceptionHandler(csx_rt_proc_exception_info_t *info);

    static void CsxPanicHandler(csx_pvoid_t context);

    /** \fn void CleanupAndExitNow()
      *  \details
      *  This function is called to cleanup, close the log, and exit pronto.
      */
    void CleanupAndExitNow(enum Mut_test_status_e status);

    void GenerateDumpFile(csx_rt_proc_exception_info_t *exception);

    bool shouldEnterDebugger(); // Return false if -nodebugger was seen on command line
    bool shouldGenerateCore(); // Return true if -core was seen on command line
    bool shouldAbortOnTimeout();
};

#endif
