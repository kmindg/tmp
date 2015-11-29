/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_test_control.cpp
 ***************************************************************************
 *
 * DESCRIPTION: MUT/C++ implementation file
 * class holds a few control points that are generally used in mut_control,
 *  accessed through global mut_control variable
 *
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/2010 Created JD
 **************************************************************************/

#include "mut_test_control.h"
#include "mut_test_listener.h"
#include "mut_thread_test_runner.h"
#include "mut_process_test_runner.h"
#include "mut_status_forwarder_runner.h"
#include "mut_test_entry.h"
#include "EmcPAL_Misc.h"

Mut_test_control::Mut_test_control(Mut_log *log)
: mut_log(log), current_test_runner(NULL), current_suite(NULL),
  end(0),start(0),mutExecutionStatus(0), mPhys_topology(NULL) {
    suite_list = new ListOfSuites();

    // Keep the mutex created for use during abort handling
    EmcpalMutexCreate(EmcpalDriverGetCurrentClientObject(), &mMutex, "mutControlMutex");
};

Mut_test_control::~Mut_test_control() {
    delete suite_list;
    EmcpalMutexDestroy(&mMutex); // Finally destroy the Mutex
    if(mPhys_topology) {
        csx_p_phys_topology_info_free(mPhys_topology);
    }

}

void Mut_test_control::process_test_error(enum Mut_test_status_e status) {
    current_test_runner->setTestStatus(status);
}

/** \fn Mut_testsuite *find_testsuite(char *name)
 *  \param name - symbolic name of the testsuite
 *  \details
 *  This function searches linked list of testsuites for testsuite with
 *  given name. If found returns pointer to this testsuite, NULL otherwise.
 */
Mut_testsuite *Mut_test_control::find_testsuite(char *name)
{
    vector<Mut_testsuite *>::iterator suite = suite_list->get_list_begin();
    while (suite != suite_list->get_list_end())
    {
        if (strcmp((*suite)->get_name(), name) == 0)
            {
                return *suite;
            }
        suite++;
    }
    return NULL;
}

mut_execution_t Mut_test_control::get_mode(){
    if (list_mode.get())
    {
        return MUT_TEST_LIST;
    }
    else if (info_mode.get())
    {
        return MUT_TEST_INFO;
    }
    else
        return MUT_TEST_EXECUTE;
}

int Mut_test_control::get_start(){
    return start;
}

int Mut_test_control::get_end(){
    return end;
}

void Mut_test_control::set_start(int s){
    start =s;
}

void Mut_test_control::set_end(int e){
    end = e;
}

Mut_abstract_test_runner *Mut_test_control::runfactory(Mut_testsuite *suite, Mut_test_entry *test) {
    if(isolated_mode.isSet()) {
        return new Mut_status_forwarder_runner(this, suite, test);
    }
    else if((isolate_mode.isSet() || isolate_mode.get()) && suite->test_list->size() != 1) {
        /*
         * .isSet() is true when -isolate was seen on the command line
         * .get() is true when mut_isolate_tests(TRUE) was called
         *
         * Execute tests in a separate process
         * when there is more than one test in a suite
         * and isolated execution has been requested
         */
        return new Mut_process_test_runner(this, suite, test);
    }
    else {
    return new Mut_thread_test_runner(this, suite, test);
    }
}

