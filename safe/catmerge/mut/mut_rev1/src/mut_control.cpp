/***************************************************************************
 * Copyright (C) EMC Corporation 2007 - 2015
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
 *
 **************************************************************************/


//#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <conio.h>
#include <time.h>
#include <string.h>
//#include <ctype.h>
#include "windows.h"
#include <vector>
using namespace std;
#include "mut_cpp.h"
#include "EmcPAL_UserBootstrap.h"
#include "EmcPAL_Misc.h"
#include "ktrace.h"
#include "ktrace_structs.h"

#include "mut_private.h"
#include "mut_sdk.h"
#include "mut_test_entry.h"
#include "mut_test_control.h"
#include "mut_log.h"
#include "mut_config_manager.h"
#include "mut_options.h"
#include "CrashHelper.h"
#include "simulation/ArrayDirector.h"
#include "simulation/PersistentCMMMemory.h"

int Mut_test_entry::global_test_counter = 0;
MUT_API Mut_test_control *mut_control;
const char mut_status_error_vectored_exception[] = "ERROR (EXCEPTION caught in vectored exception handler)";

static bool unhandledExceptionHandlingEnabled = false;
static csx_rt_proc_unhandled_exception_callback_t pUnhandledExceptionHandler = NULL;

//local static functions

static int mut_run_testsuite_once(Mut_testsuite *suite, int index);
static int mut_exit_monitor_entered = FALSE;

static void CSX_GX_RT_DEFCC mut_exit_monitor(csx_rt_proc_cleanup_context_t context)
{
    if(!mut_exit_monitor_entered)
    {
        mut_exit_monitor_entered = TRUE;
        if(mut_control->current_suite != NULL)
        {
            /*
             * _mut_run_testsuite sets mut_control->current_suite back to NULL
             * when testing is complete.  
             * As such, when current_suite is not NULL  and the monitor is entered
             * then test code is explicitly calling exit()
             * Trigger debugger to allow debugging
             */
            EmcpalDebugBreakPoint();
        }
    }
}

// local static data
static mut_notfication_callback p_notification_callback = NULL;

/** \fn void
         mut_register_event_notification_callback(mut_notfication_callback)
    \details perform progress notification
*/
MUT_API void mut_register_event_notification_callback(mut_notfication_callback pCallback)
{
    p_notification_callback = pCallback;
}


/** \fn BOOL mut_vetoable_progress_notification(mut_notification_t, Mut_testsuite *, Mut_test_entry *))
    \details perform progress notification
*/
static BOOL mut_vetoable_progress_notification(mut_notification_t state, Mut_testsuite *curent_suite, Mut_test_entry *current_test)
{
    unsigned index = (current_test != NULL) ? current_test->get_test_index() : 0xffffffff;

    if(p_notification_callback != NULL)
    {
        return (*p_notification_callback)(state, index);
    }
    else 
    {
        return TRUE;
    }

}

/** \fn int mut_str_in_list(const char *list, const char *str, int index)
 *  \param list - pointer to comma separated list of values
 *  \param str - pointer to the string we would look in the list
 *  \param index - if index is positive we would try to find it in the list
 *  as well
 *  \return TRUE if list == NULL or if str|index are found in the list,
 *  FALSE otherwise
 *  \details
 *  This function is used during test selection. If option -run_tests LIST was
 *  supplied on the command line run_tests field of mut_control structure
 *  contains pointer to the LIST, that can contain symbolic test names, test
 *  numbers or both. During test execution this function is called with str
 *  equal to test symbolic name and index to test number. If either of those
 *  is found in the list test should be executed. Otherwise test should be
 *  skipped. The same function is used for testsuite selection. Testsuites
 *  do not have numbers, so in this case -1 should be supplied as index value.
*/
static int mut_str_in_list(const char *list, const char *str, int index)
{
    int start = 0;
    int length = 0;
    char c = ' ';
    
    // Increasing the buffer size from 32 to 128
    // to accomodate some of the test names.
    char buf[128];

    if (list == NULL) return TRUE;
    while(c != '\0')
    {
        do 
        {
            c = list[start + length++];
        } while (!(c == '\0' || c == ','));

        if ((length - 1 == strlen(str)) &&  strncmp((char *)(list + start), str, length - 1) == 0)
        {
            return TRUE;
        }

        if (index >= 0)
        {
            strncpy(buf, (char *)(list + start), length - 1);
            if (isdigit(buf[0]))
            {
                buf[length - 1] = 0;
                if (atoi(buf) == index)
                {
                    return TRUE;
                }
            }
        }
        start += length;
        length = 0;
    }

    return FALSE;
}

/** \fn void mut_process_test_failure(void)
 *  \details, if the test fails, go straight into debugger/or core dump and abort 
 */
void mut_process_test_failure(void)
{
    /* Register test failure */
    mut_control->current_test_runner->setTestStatus(MUT_TEST_FAILED);
    KvPrint("MUT_LOG: Mut_control::mut_process_test_failure() - Test status set to MUT_TEST_FAILED");

    EmcpalDebugBreakPoint(); //If -autotestmode is set, this will raise an (unhandled) exception, core dump and exit
    mut_control->CleanupAndExitNow(MUT_TEST_FAILED);

    return;
}



static bool _mut_init_called = false; /* TRUE when mut_init() has been already called */


/* doxygen documentation for this function can be found in mut.h */
MUT_API void mut_init(int argc, char **argv)
{
    Arguments::resetArgs(argc,argv);
    Mut_config_manager *mcm;
    Master_IcId_option *ic_id_option;
    Cache_Size_option *cache_size_option;
    csx_ic_id_t grman_ic_id;
    csx_status_e status;

    if (_mut_init_called) /* Check if mut_init() has been already called */
    {
        printf("[MUT user program error: mut_init() has been called more than once]");
        
        // Trap into debugger so it is easy to find out where the extra call made
        EmcpalDebugBreakPoint(); // In autotestmode, this will generate an exception, coredump and exit 
        exit(1); /* The program shall not be executed longer as its source is not correct */
    }

    _mut_init_called = true;
    mut_control = new Mut_test_control(new Mut_log()); // this pointer is globally avail
    
    Mut_test_entry::reset_global_test_counter();// static function to reset static counter

    ic_id_option = Master_IcId_option::getCliSingleton();

    mcm = new Mut_config_manager(argc, argv); // option parsing done here, note also the config manager goes out of scope and is not available once mut_init exits.
    
    // See if we can get the Process Affinity here and the numa information.
    // Get number of cpus being used so we can set up correct delays
    if(csx_p_phys_topology_info_query(&mut_control->mPhys_topology) != CSX_STATUS_SUCCESS) {
        printf("[MUT user program error: mut_init() cannot determine test topology]");
        // Trap into debugger so it is easy to find out where the extra call made
        EmcpalDebugBreakPoint(); // In autotestmode, this will generate an exception, coredump and exit 
        exit(1); /* The program shall not be executed longer as its source is not correct */
    }

    // Validate the -cache_size option if it was specified.
    cache_size_option = Cache_Size_option::getCliSingleton();
    if(cache_size_option->get() != 0) {
        // Validate that the user has at least specified the minimum value that we support
        UINT_64 specifiedCacheSize = (UINT_64) cache_size_option->get() * (UINT_64) (1024 * 1024);
        if(specifiedCacheSize < CMM_MEMORY_SEGMENT_SIZE) {
            printf("[MUT user program error: mut_init() invalid cache_size entered must be minimum of %lld]", CMM_MEMORY_SEGMENT_SIZE/(UINT_64)(1024*1024));
            // Trap into debugger so it is easy to find out where the extra call made
            EmcpalDebugBreakPoint(); // In autotestmode, this will generate an exception, coredump and exit
            exit(1); /* The program shall not be executed longer as its source is not correct */
        }
    }

    // Take the physical affinity mask and sets its value for the global virtual
    // affinity mask.   All spawned processes will now inherit this option.
    Virtual_Affinity_option *pVAO = Virtual_Affinity_option::getCliSingleton();
    pVAO->_set((int) mut_control->mPhys_topology->affinity_mask_physical);

    // Setup the unhandled exception handler, this will only be added if -autotestmode was supplied on command line
    mut_control->Add_UnhandledExceptionHandler(); 
    
    grman_ic_id = (csx_ic_id_t) ic_id_option->get();

    if (grman_ic_id == csx_p_get_ic_id()) { 
        csx_module_context_t module_context;
#ifdef CSX_BV_ENVIRONMENT_WINDOWS
    #define GRMAN_MODULE_PATH "csx_grman.dll"
#else
    #define GRMAN_MODULE_PATH "csx_grman.so"
#endif
        status = csx_ci_module_create(csx_ci_container_context_get(), "csx_grman", "CSX GLOBAL resource manaager", GRMAN_MODULE_PATH, "peer_support_disabled=1 shm_support_disabled=1", CSX_NULL, CSX_NULL, CSX_NULL, CSX_NULL, &module_context);
        CSX_ASSERT_SUCCESS_H_RDC(status);
        status = csx_ci_module_invoke_load(csx_ci_container_context_get(), module_context);
        CSX_ASSERT_SUCCESS_H_RDC(status);
        status = csx_ci_module_invoke_start(csx_ci_container_context_get(), module_context);
        CSX_ASSERT_SUCCESS_H_RDC(status);
    }
    status = csx_p_grman_connection_establish(CSX_MY_MODULE_CONTEXT(), grman_ic_id);
    if (CSX_FAILURE(status)) {
        CSX_ASSERT_STATUS_H_RDC(CSX_STATUS_ALREADY_CONNECTED, status);
    }

    if(mut_control->no_fail_on_timeout.get()) {
        ArrayDirector::getSimulationDirector()->set_flag(NO_FAILURE_ON_TIMEOUT, 1);
    }
    else {
        ArrayDirector::getSimulationDirector()->set_flag(NO_FAILURE_ON_TIMEOUT, 0);
    }

    mut_control->set_start(mcm->get_start()); // test start and end ranges are not true parameters and handled separately
    mut_control->set_end(mcm->get_end());

    delete mcm; // once the options are parsed, this is not needed anymore -- now that the help
            // option instance has been moved to mut_test_control.h

    mut_control->mut_log->mut_log_init();  // can't init these until after parameters have been parsed

    if (mut_control->monitorexit.get())
    {
        mut_exit_monitor_entered = FALSE;
        csx_rt_proc_cleanup_handler_register((void *)mut_exit_monitor, mut_exit_monitor, NULL);
    }

    mut_control->mut_log->mut_log_prepare();
//  still unsure about this ktrace issue in main, some tests may find themselves with no ktrace unless they call ktrace init themselves
    KtraceInit();
    KtraceSetPrintfFunc(&mut_ktrace);
    
    // next line belongs elsewhere, but where?  keep thinking ...
    strcat(console_suite_summary,"==== Suite Summary ====\n");
    return;
}

MUT_API void set_global_timeout(unsigned long timeout) {
    //passed value needs to be propogated back to test control.
    // timeout in milliseconds.
    mut_control->timeout.set(timeout);
}

MUT_API BOOL mut_isTimeoutSet(){
    return mut_control->timeout.isSet() ? 1 : 0;
}

MUT_API void  mut_isolate_tests(BOOL b) {
    mut_control->isolate_mode._set(b ? TRUE: FALSE);
}

MUT_API void mut_add_user_suite(Mut_testsuite *suite){
    suite->set_main_test_thread_handle(NULL);

    mut_control->suite_list->push_back(suite);

}

/* doxygen documentation for this function can be found in mut.h */
MUT_API mut_testsuite_t * mut_create_testsuite(const char *name)
{
    // must use mut_testsuite_t rather than Mut_testsuite because this is visible in mut.h, for C code
    return mut_create_testsuite_advanced(name, NULL, NULL);
}

MUT_API mut_testsuite_t  * mut_create_testsuite_advanced(const char *name, mut_function_t setup, mut_function_t tear_down)
{
    // must use mut_testsuite_t rather than Mut_testsuite because this is visible in mut.h, for C code
    Mut_testsuite *suite = new Mut_testsuite(name, setup, tear_down);
    if (suite == NULL)
    {
        MUT_INTERNAL_ERROR(("Can not allocate memory for suite %s", name));
    }

    suite->set_main_test_thread_handle(NULL);

    mut_control->suite_list->push_back(suite);
    
    return (mut_testsuite_t *) suite;
}

MUT_API void mut_add_class(mut_testsuite_t *suite, 
                           const char *test_id, 
                           CAbstractTest *instance,
                           unsigned long time_out,
                           const char* sd,
                           const char* ld)
{
    mut_add_class((Mut_testsuite *)suite, test_id, instance, time_out, sd, ld);
}


MUT_API void mut_add_class(Mut_testsuite *suite, 
                   const char *test_id, 
                   CAbstractTest* instance,
                   unsigned long time_out,
                   const char* sd,
                   const char* ld) //
{ 
     
    if(sd != NULL) {
        instance->set_short_description(sd);
    }
    if(ld != NULL) {
        instance->set_long_description(ld);
    }

    // run_tests has a comma separated list of numbers or names or both
    // need to make sure current test is on that list 
    if (mut_str_in_list(mut_control->run_tests.get(), test_id, Mut_test_entry::get_global_test_counter()) &&  
          (mut_control->get_start() == -1 || Mut_test_entry::get_global_test_counter() >= mut_control->get_start() && 
           Mut_test_entry::get_global_test_counter() <= mut_control->get_end()))
    {
        // test_entry will be created with the time_out passed, if not specified default value
        // will be used
        Mut_test_entry *current_test = new Mut_test_entry(instance,test_id,time_out); 

        if (current_test == NULL)
        {
            MUT_INTERNAL_ERROR(("Failed to allocate memory for test entry \"%s\"", test_id)) ; 
        }
        
        suite->test_list->push_back(current_test);
    } 
    Mut_test_entry::inc_global_test_counter();
    return;
} 

// legacy C wrapper for mut_add_test_with_description
MUT_API void mut_add_test_with_description(mut_testsuite_t *suite, const char *test_id,
        mut_function_t test,
        mut_function_t startup, mut_function_t teardown, const char* short_desc, const char * long_desc)
{
    mut_add_test_with_description((Mut_testsuite *)suite,test_id,test,startup,teardown,short_desc,long_desc);
}

MUT_API void mut_add_test_with_description(Mut_testsuite *suite, const char *test_id,
        mut_function_t test,
        mut_function_t startup, mut_function_t teardown, const char* short_desc, const char * long_desc)
{ 
    mut_add_class(suite, test_id, new CTest(test, startup, teardown, short_desc, long_desc));
}

static int mut_run_testsuite_helper(Mut_testsuite *suite, int iteration)
{
    int status = 0;
    if(mut_vetoable_progress_notification(MUT_SUITE_START, suite, NULL))
    {
        status = mut_run_testsuite_once(suite, iteration);
    }
    (void)mut_vetoable_progress_notification(MUT_SUITE_END, suite, NULL);

    return status;
}

MUT_API void init_arguments(int argc, char **argv){
    Arguments::init(argc,argv);
}

MUT_API void reset_arguments(int argc, char **argv){
    Arguments::resetArgs(argc,argv);
}

/** To minimum change and avoid regression issue, 
 *  renamed the original mut_run_testsuite to mut_run_testsuite_once
 *  To make logic uncoupled,
 *  add the run multiple iteration logic into mut_run_testsuite
 */
// legacy C wrapper for mut_run_testsuite
MUT_API int mut_run_testsuite(mut_testsuite_t *suite)
{
    return mut_run_testsuite ((Mut_testsuite *)suite);
}

MUT_API int mut_run_testsuite(Mut_testsuite *suite)
{
    int status = 0;
    int index;
    
    switch(mut_control->iteration.get()) {
        case ITERATION_ENDLESS: // endless, don't reset the log
            while(1) {
               status = mut_run_testsuite_helper(suite,0);
            }
        
        
        default:
            for(index = 0; index< mut_control->iteration.get();index++) {
                status = mut_run_testsuite_helper(suite, index); // reset log only on first iteration
            }
    }

    mut_control->mutExecutionStatus = mut_control->mutExecutionStatus + status;
    
    mut_control->mut_log->close_log();
    
    return status;
}

MUT_API int get_executionStatus(){
    return mut_control->mutExecutionStatus;
}

/* doxygen documentation for this function can be found in mut.h */
static int mut_run_testsuite_once(Mut_testsuite *current_suite, int index)
{
    int exit_status = 0;
    //int test_status;
    vector<Mut_test_entry *>::iterator current_test;
    Mut_testsuite *old_suite = NULL;
    bool continue_testing;
    
    /* If list of testsuites to run was supplied and this testsuite is not there - return */
    if (!mut_str_in_list(mut_control->run_testsuites.get(), current_suite->get_name(), -1))
    {
        return exit_status;
    }

    /* keep pointer for previous testsuite in case of recursive suites */
    old_suite = mut_control->current_suite;

    /* set pointer in global structure */
    mut_control->current_suite = current_suite;
    if (mut_control->get_mode() == MUT_TEST_LIST)
    {
        /* If we are in list mode run through the test list and print name and number of each test */
        current_suite->list_tests();
        return exit_status; /* listing is done - return */
    }
    else if (mut_control->get_mode() == MUT_TEST_INFO)
    {
        /* If we are in info mode run through the test list and print description */
        current_suite->info();
        return exit_status; /* info is done - return */
    }

    /* get head of tests linked list */
    current_test = current_suite->test_list->get_list_begin();

    if(!current_suite->test_list->empty()) 
    {
        if( index==0 )
        {
            mut_control->mut_log->create_log(current_suite->get_name());
        }
    
        mut_control->mut_log->mut_report_testsuite_starting(current_suite->get_name(), index, mut_control->iteration.get() );
        //call suite startup function here.  
        current_suite->suiteStartUp();
    

        continue_testing = true;
        while(current_test != current_suite->test_list->get_list_end() && continue_testing)
        {
            if(mut_vetoable_progress_notification(MUT_TEST_START, current_suite, *current_test))
            {
                /* 
                 * Execute Test
                 */
                mut_control->current_test_runner = mut_control->runfactory(current_suite, *current_test);
                mut_control->current_test_runner->run();
                delete mut_control->current_test_runner;
                mut_control->current_test_runner = NULL;
                
                // Move to the next test
                current_test++;
            }
            (void)mut_vetoable_progress_notification(MUT_TEST_END, current_suite, 
                                                     (current_test!=current_suite->test_list->get_list_end()) ? *current_test : NULL);
        }

        current_suite->suiteTearDown();

    // Report the suite end
        mut_control->mut_log->mut_report_testsuite_finished(current_suite->get_name(), (mut_testsuite_t *)current_suite);
        mut_control->mut_log->mut_report_test_summary(current_suite->get_name(), (mut_testsuite_t *)current_suite, mut_control->iteration.get());
        
    }
    // Print the suite summary buffer (buffer is not updated when looping)
    if(current_suite->is_last(mut_control->suite_list) && (mut_control->iteration.get() == 1))
    {
        printf("%s",console_suite_summary);
    }

  /*  if (exit_status)
    {
        exit(exit_status);
    }
  */
    /* restore current_suite since it could be redefined by nested suites */
    mut_control->current_suite = old_suite;

    return exit_status;
}


// c interface functions

MUT_API const char *mut_get_current_suite_name(void)
{
    return mut_control->current_suite->get_name();
}

MUT_API char *mut_get_user_option_value(const char *option_name)
{
    char *value;
    Program_Option *option = Options::get_option(option_name);
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

MUT_API BOOL mut_option_selected(const char *option_name) {
    Program_Option *option = Options::get_option(option_name);

    return (option == NULL) ? FALSE : option->isSet();
}


MUT_API BOOL mut_register_user_option(const char *option_name, int least_num_of_args, BOOL show, const char *description){ //why does this return anything??
    // argc supplied by user is one less than what option system expects.  add 1
    new mut_user_option(option_name, least_num_of_args+1, (show ? true:false), description); // convert show from BOOL(int) to bool
    return 1;
}

mut_printf_func_t mut_get_user_trace_printf_func(void)
{
    return mut_ktrace;
}

MUT_API csx_ic_id_t mut_get_master_ic_id (
    csx_void_t)
{
    Master_IcId_option *ic_id_option;
    ic_id_option = Master_IcId_option::getCliSingleton();

    return (csx_ic_id_t) ic_id_option->get();
}

MUT_API int mut_get_specified_cache_size(csx_void_t)
{
    Cache_Size_option* cs_option;
    cs_option = Cache_Size_option::getCliSingleton();

    return (cs_option->get());
}

MUT_API char* mut_get_specified_platform(csx_void_t)
{
    Platform_Config_option* platf_option;
    platf_option = Platform_Config_option::getCliSingleton();

    return (platf_option->get());
}

/*!**************************************************************
 * mut_pause()
 ****************************************************************
 * @brief
 *  pend the test in order to observe the output of the test at some timepoint.
 *
 * @param None.
 *
 * @return None.
 *
 * @author
 *  3/11/2011 - Created. zphu
 *
 ****************************************************************/
MUT_API void mut_pause()
{
    if (!mut_control->mut_is_test_pause())
        return;

    mut_printf(MUT_LOG_LOW, "%s: Test paused, press Enter to continue...", __FUNCTION__);
    while('\n' != getchar())
    {
        continue;
    }
}


/** \fn void enable_unhandled_exception_handling(BOOL state, BOOL logMessage)
 *  \details
 *  Local/static function used to enable/disable unhandled exception handling.
 */
static void enable_unhandled_exception_handling(bool state, bool logMessage)
{
    // Log a message?
    if(logMessage == true)
        if(state == true)
            mut_printf(MUT_LOG_TEST_STATUS, "MUT framework unhandled exception handling has been enabled.");
        else
            mut_printf(MUT_LOG_TEST_STATUS, "MUT framework unhandled exception handling has been disabled.");

    // Do it
    unhandledExceptionHandlingEnabled = state;
}

/** \fn void mut_enable_framework_exception_handling()
 *  \details
 *  This function is used to enable/disable unhandled exception handling.
 *  Note: Enabling/disabling exceptions should only be done in 
 *  rare circumstances such as a test that does exception handling.
 */
void mut_enable_framework_exception_handling(BOOL state)
{
    enable_unhandled_exception_handling(state?true:false, true); // convert C TRUE to C++ true
}


void Mut_test_control::Add_UnhandledExceptionHandler()
{
    // Add the unhandled exception support, no matter what
    pUnhandledExceptionHandler = Mut_test_control::UnhandledExceptionHandler;
    csx_rt_proc_set_unhandled_exception_callback(Mut_test_control::UnhandledExceptionHandler);
    
    csx_rt_assert_set_user_panic_handler(Mut_test_control::CsxPanicHandler, NULL);

    // Enable exception handling
    enable_unhandled_exception_handling(true, false);
    KvPrint("MUT_LOG: Mut_test_control::Add_UnhandledExceptionHandler() - Unhandled Exception support added\n");
}


void Mut_test_control::Remove_UnhandledExceptionHandler()
{
    // Remove the unhandled exception handler
    if(pUnhandledExceptionHandler != NULL)
    {
        csx_rt_proc_set_unhandled_exception_callback(NULL);
        pUnhandledExceptionHandler = NULL;
    }
}

bool Mut_test_control::shouldEnterDebugger(){
    // Return false if -nodebugger is seen on command line
    return !mut_control->nodebugger.get();
}

bool Mut_test_control::shouldAbortOnTimeout(){
    // return false if -nofailontimeout is set on command line
    return !mut_control->no_fail_on_timeout.get() ;
}

bool Mut_test_control::shouldGenerateCore(){
    // Return true if -core is set on command line, else return false
    return mut_control->core.get();
}

void Mut_test_control::CsxPanicHandler(csx_pvoid_t context)
{
    CSX_UNREFERENCED_PARAMETER(context);
    if(mut_control->shouldEnterDebugger() == true) {
        KvPrint("MUT_LOG: Mut_control::CsxPanicHandler() - Test status set to MUT_TEST_FAILED");
        mut_control->current_test_runner->setTestStatus(MUT_TEST_FAILED);
        mut_control->CleanupAndExitNow(MUT_TEST_CSX_PANIC);
    }
}


// Handler for all unhandled exceptions.
csx_status_e CSX_GX_RT_DEFCC Mut_test_control::UnhandledExceptionHandler(csx_rt_proc_exception_info_t *ExceptionInfo)
{
    KvPrint("MUT_LOG: Mut_control::UnhandledExceptionHandler()");
        
    if(mut_control->shouldEnterDebugger()) {
        return CSX_STATUS_EXCEPTION_CONTINUE_SEARCH;
    }

    // tell CSX we're going to panic.  This will lower the priority of any realtime threads before the dump to prevent potential
    // priority inversion issues
    csx_rt_proc_run_panic_handlers();

    // Write a dump file
    KvPrint("MUT_LOG: Mut_control::UnhandledExceptionHandler() - Acquiring lock to proceed to GenerateDumpFile() and CleanupAndExitNow()");
    EmcpalMutexLock(&mut_control->mMutex); // Take the lock, don't unlock, it may cause other threads to continue this path while this thread is exiting
    KvPrint("MUT_LOG: Mut_control::UnhandledExceptionHandler() - Lock acquired");
    
    // Only generate a core dump if -nodebugger was set and -core was set, otherwise just cleanup and exit
    if(mut_control->shouldGenerateCore()) {
        mut_control->GenerateDumpFile(ExceptionInfo);
    }

    // Bail out totally
    mut_control->CleanupAndExitNow(MUT_TEST_VECTORED_EXCEPTION);
 
    // Need to return something even though we are exiting before this
    return CSX_STATUS_EXCEPTION_CONTINUE_SEARCH;
}


void Mut_test_control::GenerateDumpFile(csx_rt_proc_exception_info_t *exception)
{
    char *pDumpFileName;
    csx_pvoid_t native_exception_info = CSX_NULL;
    KvPrint("MUT_LOG: Mut_control::GenerateDumpFile()");
    // Write a dump file
    pDumpFileName = mut_control->mut_log->get_dump_file_name(); // Use the default name
    mut_printf(MUT_LOG_TEST_STATUS, "Writing dump file to: %s", pDumpFileName);
    if (CSX_NOT_NULL(exception)) {
        native_exception_info = exception->native_exception_info;
    }
    CreateDumpFile(pDumpFileName, native_exception_info); /* SAFEMESS */    
}

void Mut_test_control::CleanupAndExitNow(enum Mut_test_status_e status)
{
        KvPrint("MUT_LOG: Mut_control::CleanupAndExitNow()");
        // Set the test error
        mut_control->process_test_error(status);

        // Calculate how many tests were not executed by scanning the rest of test_list
        mut_control->mut_log->ReportIncompleteTests(mut_control->current_suite,(Mut_test_entry *)mut_control->current_test_runner->getTest());

        // End the test
        mut_control->mut_log->mut_report_test_finished(mut_control->current_suite,(Mut_test_entry *)mut_control->current_test_runner->getTest(),\
                                                       mut_control->current_test_runner->getTest()->get_status_str());

        // Finish the suite
        mut_control->mut_log->mut_report_testsuite_finished(mut_control->current_suite->get_name(), (mut_testsuite_t *)mut_control->current_suite); // @FIXME

        // Close the summary
        mut_control->mut_log->mut_report_test_summary(mut_control->current_suite->get_name(), (mut_testsuite_t *)mut_control->current_suite, mut_control->iteration.get()); //@FIXME
    
        mut_control->mut_log->close_log();

        // Handling the case where a CSX_PANIC is detected and is in progress during test Setup/Test/Teardown.
        // We don't want a case where suite teardown could potentially encounter another CSX_PANIC. Bypass that and exit right away.
        // This might leave FBE SP sim A/B windows open when fbe_test.exe -nodebugger runs a test and the test panics.
        if (CSX_IS_FALSE(csx_rt_assert_get_is_panic_in_progress())) {
            
            // Call suiteTearDown to cleanup
            CSX_TRY {   
                KvPrint("MUT_LOG: Mut_control:: CSX_TRY, starting suiteTearDown()");
                mut_control->current_suite->suiteTearDown();
                KvPrint("MUT_LOG: Mut_control:: SuiteTearDown successful"); // If it returns here, teardown is assumed to be successful
            }
            CSX_CATCH(mut_c_exception_filter, CSX_NULL) {
                // If suite teardown fails, catch the exception (do not go to debugger)
                KvPrint("MUT_LOG: Mut_control:: Exception caught in CSX_CATCH during suiteTeardown()");

                KvPrint("Exiting"); // In any case, bail out totally. 
                CSX_EXIT(1);
            
            }CSX_END_TRY_CATCH
        } 
        if (status != MUT_TEST_CSX_PANIC) { /* if this is a CSX panic the panic path will take care of the exit for us */
            CSX_EXIT(1);
        }
}

// Return a flag to let CSX_CATCH handler take over the control
csx_status_e Mut_test_control::mut_c_exception_filter(csx_rt_proc_exception_info_t *exception, csx_rt_proc_catch_filter_context_t context)
{
    CSX_UNREFERENCED_PARAMETER(context);
    // Goto the debugger if -nodebugger was not supplied to the program
    if(mut_control->shouldEnterDebugger()) {
        KvPrint("-nodebugger was not set, exception will invoke a debugger");
        return CSX_STATUS_EXCEPTION_CONTINUE_SEARCH;
    }
    
    // Only generate a core dump if -nodebugger was set and -core was set, otherwise just cleanup and exit
    if(mut_control->shouldGenerateCore()) {
        // if -core was seen on the command line, then generate a coredump
        mut_control->GenerateDumpFile(exception);
    }
            
    KvPrint("-nodebugger was set, exception will be caught in the handler");
    return CSX_STATUS_EXCEPTION_EXECUTE_HANDLER;
}

#include "EmcUTIL_CsxShell_Interface.h"

extern "C" csx_bool_t EMCUTIL_SHELL_DEFCC emcutil_dll_startup(void)
{
    return CSX_TRUE;
}

extern "C" csx_bool_t EMCUTIL_SHELL_DEFCC emcutil_dll_shutdown(void)
{
    return CSX_TRUE;
}

