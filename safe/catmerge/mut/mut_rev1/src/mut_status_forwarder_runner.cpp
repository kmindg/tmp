/***************************************************************************
 * Copyright (C) EMC Corporation 2007 - 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_status_forwarder_runner.cpp
 ***************************************************************************
 *
 * DESCRIPTION: Implementation of the mut_status_forwarder_runner class.  This
 *              class forwards the status of a test to another process
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *
 **************************************************************************/

# include "mut_status_forwarder_runner.h"

# include "mut_test_control.h"
# include "mut_thread_test_runner.h"
# include "mut_testsuite.h"
# include "mut_test_entry.h"

Mut_status_forwarder_runner::Mut_status_forwarder_runner(Mut_test_control *mut_control,
                                Mut_testsuite *current_suite,
                                Mut_test_entry *current_test)
: Mut_thread_test_runner(mut_control, current_suite, current_test), 
  mTestListener(new Mut_test_listener(mut_control)), mControl(mut_control), mTest(current_test) {
    mControl->mut_log->registerListener(mTestListener);
}
Mut_status_forwarder_runner::~Mut_status_forwarder_runner() {
    mControl->mut_log->registerListener(NULL);
    delete mTestListener;
}

void Mut_status_forwarder_runner::setTestStatus(enum Mut_test_status_e status) {
    mTestListener->setTestStatus(status);
    Mut_thread_test_runner::setTestStatus(status);
}

bool Mut_status_forwarder_runner::logTestStatus() {
    /*
     * The main test process has already reported test start/end
     * Skip reporting test start/end in the isolated process
     * as this is redundant
     */
    return false;
     
}

Mut_test_entry * Mut_status_forwarder_runner::getTest() {
    return mTest;
}

