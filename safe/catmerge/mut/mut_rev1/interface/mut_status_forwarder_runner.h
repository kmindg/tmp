/***************************************************************************
 * Copyright (C) EMC Corporation 2007 - 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_status_forwarder_runner.h
 ***************************************************************************
 *
 * DESCRIPTION: Class definition of mut_status_forwarder_runner.  This
 *              class forwards the status of a test to another process
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *
 **************************************************************************/

# include "mut_thread_test_runner.h"
# include "mut_test_listener.h"

class Mut_test_control;

class Mut_status_forwarder_runner: public Mut_thread_test_runner {
public:
    Mut_status_forwarder_runner(Mut_test_control *mut_control,
                                Mut_testsuite *current_suite,
                                Mut_test_entry *current_test);
    ~Mut_status_forwarder_runner();
    virtual void setTestStatus(enum Mut_test_status_e status);

    bool logTestStatus();
    Mut_test_entry *getTest(); // returns the current test

private:
    Mut_test_listener *mTestListener;
    Mut_test_control *mControl;
    Mut_test_entry *mTest;
};
