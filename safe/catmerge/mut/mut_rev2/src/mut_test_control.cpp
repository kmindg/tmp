/*
 * mut_test_control.cpp
 *
 *  Created on: Sep 21, 2011
 *      Author: londhn
 */
#include "mut_test_control.h"
#include "mut_thread_test_runner.h"
#include "string.h"

Mut_test_control::Mut_test_control(Mut_log *log):mut_log(log), current_test_runner(NULL), current_suite(NULL),
end(0), start(0), mutExecutionStatus(0), mPhys_topology(NULL){
    suite_list = new ListOfSuites();
}

Mut_test_control::~Mut_test_control() {
    delete suite_list;
    if(mPhys_topology) {
        csx_p_phys_topology_info_free(mPhys_topology);
    }
}

void Mut_test_control::process_test_error(enum Mut_test_status_e status)
{
    current_test_runner->setTestStatus(status);
}
/** \fn DefaultSuite *find_testsuite(char *name)
 *  \param name - symbolic name of the testsuite
 *  \details
 *  This function searches linked list of testsuites for testsuite with
 *  given name. If found returns pointer to this testsuite, NULL otherwise.
 */
DefaultSuite *Mut_test_control::find_testsuite(char *name)
{
    vector<DefaultSuite *>::iterator suite = suite_list->get_list_begin();
    while (suite != suite_list->get_list_end())
    {
        if (strcmp((*suite)->getName(), name) == 0)
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
    else if (info_mode.get()){
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
    start = s;
}

void Mut_test_control::set_end(int e){
    end = e;
}

Mut_abstract_test_runner *Mut_test_control::runfactory(DefaultSuite *suite,DefaultTest *test){
    return new Mut_thread_test_runner(this,suite,test);
}



