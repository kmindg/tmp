/***************************************************************************
 * Copyright (C) EMC Corporation 2007 - 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_thread_test_runner.h
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
#ifndef __MUTTHREADTESTRUNNER__
#define __MUTTHREADTESTRUNNER__

#include "EmcPAL.h"
#include "EmcPAL_DriverShell.h"
#include "EmcPAL_Misc.h"
#include "K10Assert.h" 
#include "mut_abstract_test_runner.h"
#include "csx_ext.h"

class Mut_test_control;
class Mut_testsuite;
class Mut_test_entry;
class Mut_thread_test_runner;
enum Mut_test_status_e;

// To check race condition during AbortOnTimeout
enum test_stage {
    MUT_SETUP_AND_TEST,
    MUT_TEARDOWN
};

// This will be passed to the timeout handler when timeout occurs
struct timeout_data {
    Mut_thread_test_runner *context;
    enum test_stage stage;
};

class Mut_thread_test_runner: public Mut_abstract_test_runner {
public:
    Mut_thread_test_runner(Mut_test_control *mut_control,
                            Mut_testsuite *current_suite,
                            Mut_test_entry *current_test);
    ~Mut_thread_test_runner();
    const char *getName();

    void run();

    virtual void setTestStatus(enum Mut_test_status_e status);

    virtual bool logTestStatus();

    /** \fn void mut_run_setup_and_test(void *context)
     *  \param context - pointer to the test_entry_t structure
     *  \details
     *  This function is used for starting separate thread with test setup and
     *  actual test logic.
     */
    static void mut_run_setup_and_test(void *context);

    /** \fn void mut_run_tear_down(void *context)
     *  \param context - pointer to the Mut_test_entry structure
     *  \details
     *  This function is used for starting separate thread with test teardown.
     *  It is very similiar to mut_run_setup_and_test().
     */
     static void mut_run_tear_down(void *context);
     
      
     /** \fn void getTestStage()
       *  \details
       *  This function is called to get the stage the test is in: which can be either MUT_SETUP_AND_TEST or MUT_TEARDOWN 
       */

     enum test_stage getTestStage() {
        return mTestStage;
     }

     /** \fn void setTestStage()
       *  \details
       *  This function is called to set the stage (setup/teardown) the test is in: which can be either MUT_SETUP_AND_TEST or MUT_TEARDOWN 
       */

     void setTestStage(enum test_stage stage) {
        mTestStage = stage;
     }

     Mut_test_control *getControl();
     Mut_testsuite    *getSuite();
     Mut_test_entry   *getTest();

private:
    static void AbortOnTimeout(EMCPAL_TIMER_CONTEXT context);
    
    Mut_test_control    *mControl;
    Mut_testsuite       *mSuite;
    Mut_test_entry      *mTest;

    EMCPAL_MONOSTABLE_TIMER mTimer; // Timer to monitor timeout	
    struct timeout_data mOnTimeout;
    enum test_stage mTestStage; // to check race condition
    EMCPAL_MUTEX mMutex;
};

#endif
