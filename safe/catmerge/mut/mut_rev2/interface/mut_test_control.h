/*
 * mut_test_control.h
 *
 * DESCRIPTION: This class holds all the control data for mut
 *      It's treated like a singleton, initialized once in mut_control and
 *      generally referenced as needed
 * FUNCTIONS:
 *
 * Created on: Sep 21, 2011
 *      Author: londhn
 */

#ifndef MUT_TEST_CONTROL_H_
#define MUT_TEST_CONTROL_H_

#include <vector>
#include <string>

#include "mut/defaultsuite.h"
#include "mut/defaulttest.h"

#include "mut_sdk.h"
#include "mut_config_manager.h"
#include "mut_options.h"
#include "mut_log.h"
#include "mut_abstract_test_runner.h"

#include "EmcPAL.h"

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

    vector<DefaultSuite *>::iterator get_list_begin() { return list.begin(); }
    vector<DefaultSuite *>::iterator get_list_end()   { return list.end(); }
    void push_back(DefaultSuite *v) { try {list.push_back(v);} catch(...){MUT_INTERNAL_ERROR(("ListOfSuites push_back exception"));} }
    void clear() { list.clear(); }
    unsigned int size() { return (unsigned int)list.size();}
    bool empty() { return list.empty(); }
    DefaultSuite *back() {return list.back();}

private:
    vector<DefaultSuite *> list;
};

class ListOfTests {
public:
    ListOfTests() {};
    ~ListOfTests() {};

    vector<DefaultTest *>::iterator get_list_begin() { return list.begin(); }
    vector<DefaultTest *>::iterator get_list_end()   { return list.end(); }
    void push_back(DefaultTest *v) { try {list.push_back(v);} catch(...){MUT_INTERNAL_ERROR(("ListOfTests push_back exception"));} }
    void clear() { list.clear(); }
    bool empty() { return list.empty(); }
    unsigned int size() {return (unsigned int)list.size();}
    DefaultTest *back() {return list.back();}

private:
    vector<DefaultTest *> list;
};

/** \class mut_test_control
 *  \details
 *  This structure holds all settings and pointers for running tests. It
 *  exists as single static instance and is initialized by mut_init function.
 */
class Mut_test_control{
private:
    int start;
    int end;
    int mutExecutionStatus;

public:
    // Not all the running options are supported yet
    // to add run_tests and run_testsuites options we need
    // to decide/plan about the threading in linux

    // adding the time out option

    mut_list_option list_mode;
    mut_info_option info_mode;
    mut_execution_option fail_on_timeout;
    mut_abort_policy_option abort;
    mut_timeout_option timeout;
    mut_run_tests_option run_tests;
    csx_p_phys_topology_info_t *mPhys_topology;

    Mut_log *mut_log;

    Mut_test_control(Mut_log *log);

    ~Mut_test_control();

    mut_execution_t get_mode();

    Mut_abstract_test_runner *current_test_runner;
    DefaultSuite *current_suite; /**< Testsuite that is currently running */
    ListOfSuites *suite_list;     /**< Testsuites are stored in a vector.*/

    /** \fn DefaultSuite *find_testsuite(char *name)
      *  \param name - symbolic name of the testsuite
      *  \details
      *  This function searches linked list of testsuites for testsuite with
      *  given name. If found returns pointer to this testsuite, NULL otherwise.
      */
    DefaultSuite *find_testsuite(char *name);

    /** \fn void process_test_error(enum Mut_test_status_e status)
      *  \param    status -  status of test
      *  \details register test error
      */
    void process_test_error(enum Mut_test_status_e status);

    int get_start();
    int get_end();

    void set_start(int s);
    void set_end(int e);

    PEMCPAL_MONOSTABLE_TIMER getTimer();

    Mut_abstract_test_runner *runfactory(DefaultSuite *suite, DefaultTest *test);
};
#endif /* MUT_TEST_CONTROL_H_ */
