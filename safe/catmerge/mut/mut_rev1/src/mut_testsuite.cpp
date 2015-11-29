/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *                                mut_testsuite.cpp
 ***************************************************************************
 *
 * DESCRIPTION: MUT/C++ Mut_testsuite class implementation file
 *
 * FUNCTIONS:
 *
 * NOTES:
 *
 * HISTORY:
 *    10/2010 Created JD
 **************************************************************************/

#include "mut_test_control.h"
#include "mut_testsuite.h"
#include "mut_test_entry.h"
#include "mut_log_private.h"

MUT_API Mut_testsuite::Mut_testsuite(const char *n, mut_function_t s, mut_function_t t)
: name(new MutString(n)), startup(s),teardown(t), test_list(new ListOfTests()){
    thread.handle = NULL;
        };

MUT_API Mut_testsuite::~Mut_testsuite()
{
    delete name;
    delete test_list;
}

void Mut_testsuite::suiteStartUp()
{
    if (startup)
    {
        (*startup)();
    }
}

void Mut_testsuite::suiteTearDown()
{
    if(teardown)
    {
        (*teardown)();
    }
}


MUT_API void Mut_testsuite::list_tests()
{
    vector<Mut_test_entry *>::iterator current_test = test_list->get_list_begin();

    /* get head of tests linked list */
    
	printf("%s: \n", get_name());
    while(current_test != test_list->get_list_end())
    {
        
        const char * desc = (*current_test)->get_test_instance()->get_short_description();

        printf("(%d) [timeout: %lu sec.] %s%s%s\n",
	       (*current_test)->get_test_index(),
	       (*current_test)->get_time_out(),
               (*current_test)->get_test_id(),
               desc ? ":    " : "",
               desc ? desc : "");

        current_test++;
    }

	return;
}

MUT_API void Mut_testsuite::info()
{
    
    vector<Mut_test_entry *>::iterator current_test = test_list->get_list_begin();
    /* get head of tests linked list */
    
    while(current_test != test_list->get_list_end())
    {
       
        printf("(%d) %s.%s\n  Short description: %s\n  Long  description: %s\n",
               (*current_test)->get_test_index(), 
               get_name(), (*current_test)->get_test_id(),
               (*current_test)->get_test_instance()->get_short_description(),
               (*current_test)->get_test_instance()->get_long_description());
    
        current_test++;
    }

	return;
}

// see if this is last in the list that's passed in
// function now returns true if there is only one suite on the list.
MUT_API bool Mut_testsuite::is_last(ListOfSuites *suite_list)
{
    return (strcmp(suite_list->back()->get_name(), get_name())== 0);
}

MUT_API void Mut_testsuite::set_main_test_thread_handle(void* h)
{
    thread.handle = h;
}

MUT_API mut_thread_t *Mut_testsuite::get_main_test_thread()
{
    return &thread;
}


MUT_API const char* Mut_testsuite::get_name()
{
    return name->c_str();
}
