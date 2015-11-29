/***************************************************************************
 * Copyright (C) EMC Corporation 2007 - 2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_thread_test_runner.cpp
 ***************************************************************************
 *
 * DESCRIPTION: MUT thread test runner definition
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *
 **************************************************************************/

# include "mut_test_control.h"
# include "mut_thread_test_runner.h"
# include "mut_testsuite.h"
# include "mut_test_entry.h"
# include "CrashHelper.h"
# include "ktrace.h"


const char mut_status_finished[]               = "Finished";
const char mut_status_started[]                = "Started ";

static Mut_thread_test_runner *current_runner = NULL;

Mut_thread_test_runner::Mut_thread_test_runner(Mut_test_control *control,
                                                Mut_testsuite *suite,
                                                Mut_test_entry *test)
: mControl(control), mSuite(suite), mTest(test), mTestStage(MUT_SETUP_AND_TEST)  {

    csx_p_memzero(&mOnTimeout, sizeof(mOnTimeout));
    
    // Create a Mutex
    EmcpalMutexCreate(EmcpalDriverGetCurrentClientObject(), &mMutex, "mutTimerMutex");

    //Create the timer, configure it to run abort procedure if the timeout is hit
   	EmcpalMonostableTimerCreate(EmcpalDriverGetCurrentClientObject(),
                                    &mTimer,
                                    "MutTimer",
                                    &Mut_thread_test_runner::AbortOnTimeout,
                                    (EMCPAL_TIMER_CONTEXT) &mOnTimeout);
}
Mut_thread_test_runner::~Mut_thread_test_runner() {
    EmcpalMonostableTimerDestroy(&mTimer); // Destroy the timer
    EmcpalMutexDestroy(&mMutex); // Finally destroy the Mutex
}

const char *Mut_thread_test_runner::getName() {
    return mTest->get_test_id();
}

Mut_test_control *Mut_thread_test_runner::getControl() {
    return mControl;
}

Mut_testsuite    *Mut_thread_test_runner::getSuite() {
    return mSuite;
}

Mut_test_entry   *Mut_thread_test_runner::getTest() {
    return mTest;
}

void Mut_thread_test_runner::setTestStatus(enum Mut_test_status_e status) {
    mTest->set_status(status);
}

bool Mut_thread_test_runner::logTestStatus() {
    // during normal execution report test status
    return TRUE;
}

// Handler for Timeout event
void Mut_thread_test_runner::AbortOnTimeout(EMCPAL_TIMER_CONTEXT context) {
    timeout_data *mOnTimeout = (struct timeout_data *) context; 
    Mut_thread_test_runner *timeoutContext = mOnTimeout->context;

    KvPrint("MUT_LOG: Mut_thread_test_runner::AbortOnTimeout() called");

    // Set test_status depending upon the stage in which the test was
    enum Mut_test_status_e test_status = mOnTimeout->stage == MUT_SETUP_AND_TEST ? MUT_TEST_TIMEOUT_IN_TEST : MUT_TEST_TIMEOUT_IN_TEARDOWN;
    csx_p_thr_disable_xlevel_validation();
    EmcpalMutexLock(&timeoutContext->mMutex); // Lock 

    /*
     * There is a very small chance that the test completes at the same time as the timeout
     * The next check confirms that the test has not exited execution, so it is a real timeout.
     */
    if(mOnTimeout->stage == timeoutContext->getTestStage()) {
        // There is no need to lock this abort path because it's already under the monitor
        if(timeoutContext->getControl()->shouldAbortOnTimeout()) {
           KvPrint("MUT_LOG: Mut_thread_test_runner::AbortOnTimeout() Calling EmcpalDebugBreakPoint()");
           KvPrint("MUT_LOG: Mut_thread_test_runner::AbortOnTimeout() to pass control to the exception handler");
           EmcpalDebugBreakPoint(); // THis will cause it to go to the unhandled exception handler and take appropriate action
           timeoutContext->getControl()->CleanupAndExitNow(test_status);
        }
        else {
            KvPrint("MUT_LOG: Mut_thread_test_runner::AbortOnTimeout() abort on timeout disabled");
        }
    }
    else {
        KvPrint("MUT_LOG: Mut_thread_test_runner::AbortOnTimeout() Test execution completed before the time entered critical region");
    }
    
    EmcpalMutexUnlock(&timeoutContext->mMutex); // Unlock
    csx_p_thr_enable_xlevel_validation();
}

void Mut_thread_test_runner::run() {
    EMCPAL_STATUS timeoutStatus;
    EMCPAL_THREAD testThread_handle;
    PEMCPAL_CLIENT pClientObject = EmcpalDriverGetCurrentClientObject();

    current_runner = this;
    mOnTimeout.context = this;

	// Default to 'unknown' test status
    setTestStatus(MUT_TEST_RESULT_UKNOWN);

    if(logTestStatus()) {
        mControl->mut_log->mut_report_test_started(mSuite, mTest);
    }

    /* Each test has 3 components: setup, test itself and teardown. Setup
     * is responsible for setting up environment, allocating necessary
     * resources etc. Teardown is doing cleanup after test is completed.
     * Since during execution of setup and test we can encounter runtime
     * exceptions and timeouts setup and test are started in separate thread
     * using mut_run_setup_and_test() wrapper and teardown function is
     * started in another thread so that we would cleanup after test despite
     * any problems that could occur.
     */
    
    /* Procedure for Setup and Test */


    EmcpalMutexLock(&mMutex); // Lock 
    timeoutStatus = EmcpalMonostableTimerStart(&mTimer,(mTest->get_time_out()) * 1000);
    FF_ASSERT(timeoutStatus == EMCPAL_STATUS_SUCCESS);

    // Create a thread to run setup_and_test
    setTestStage(MUT_SETUP_AND_TEST);
    mOnTimeout.stage = getTestStage();

    EmcpalMutexUnlock(&mMutex); // Unlock
    
    EmcpalThreadCreate(pClientObject, &testThread_handle, "mutSetupAndTestThread",&Mut_thread_test_runner::mut_run_setup_and_test,(void *)this); 
    EmcpalThreadWait(&testThread_handle);

    EmcpalMutexLock(&mMutex); // Lock
    mOnTimeout.stage=MUT_TEARDOWN;
	EmcpalMonostableTimerCancel(&mTimer); // Reset the timer
    EmcpalMutexUnlock(&mMutex); // Unlock
    
    // Destroy the Thread
    EmcpalThreadDestroy(&testThread_handle);


    EmcpalMutexLock(&mMutex); // Lock 
    timeoutStatus = EmcpalMonostableTimerStart(&mTimer, (mTest->get_time_out()) * 1000);
    FF_ASSERT(timeoutStatus == EMCPAL_STATUS_SUCCESS);

    // Create a thread to run teardown 
    setTestStage(MUT_TEARDOWN);
    EmcpalMutexUnlock(&mMutex); // Unlock
    
    // Create a Mutex
    EmcpalThreadCreate(pClientObject, &testThread_handle, "mutTeardownThread",&Mut_thread_test_runner::mut_run_tear_down,(void *) this);
    EmcpalThreadWait(&testThread_handle);
    
    EmcpalMutexLock(&mMutex); // Lock
    EmcpalMonostableTimerCancel(&mTimer); // Reset the timer
    EmcpalMutexUnlock(&mMutex); // Unlock
    
    // Destroy the Thread
    EmcpalThreadDestroy(&testThread_handle);
    
    
	// Before calling test above we set the test result to 'unknown'.
	// Now we check the test result post test:
	// If 'unknown' then it hasn't been changed during the test so the test passed.
	// If 'failed' then the test has changed it and it has a problem.
	if(mTest->get_status() == MUT_TEST_RESULT_UKNOWN){
		setTestStatus(MUT_TEST_PASSED);
	}

    mControl->mut_log->mut_report_test_finished(mSuite, mTest, mTest->get_status_str(), logTestStatus());
    
    return;
}

void Mut_thread_test_runner::mut_run_setup_and_test(void *context)
{
    Mut_thread_test_runner *testRunner = ((Mut_thread_test_runner *)context);

    // Timeout exception will be handled by AbortOnTimeout Handler
    // All other exceptions will be handled by the Unhandled Exception Handler in mut_control.cpp
    // Startup and Test are guaranteed to be non-null so there's no need to check before calling
    CAbstractTest *ctest = testRunner->getTest()->get_test_instance();
    ctest->StartUp();
    ctest->Test();

    return;
}

void Mut_thread_test_runner::mut_run_tear_down(void *context)
{
    Mut_thread_test_runner *testRunner = ((Mut_thread_test_runner *)context);

    // Timeout exception will be handled by AbortOnTimeout Handler
    // All other exceptions will be handled by the Unhandled Exception Handler in mut_control.cpp
    testRunner->getTest()->get_test_instance()->TearDown();
    
    return;
}
