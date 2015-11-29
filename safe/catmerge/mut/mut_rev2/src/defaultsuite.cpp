/*
 * DefaultSuite.cpp
 *
 *  Created on: Sep 21, 2011
 *      Author: londhn
 */
#include "mut_test_control.h"
#include "mut/defaultsuite.h"
#include "mut/defaulttest.h"

#include <string.h>
#include <stdio.h>

using namespace std;

extern Mut_test_control *mut_control;

static int suiteExecutionStatus = 0;

int DefaultSuite::test_counter = 0;

DefaultSuite::DefaultSuite(const char *name){
	suiteName = name;
	test_list = new ListOfTests();
	
	mut_control->mut_log->log_message("Mut Suite created \n");
};

DefaultSuite::~DefaultSuite()
{
    delete suiteName;
    delete test_list;
}

// This method will execute the tests in the suite, currently
// this methods acts as a wrapper to mut_run_testsuite.
MUT_API int DefaultSuite::run_all_tests()
{
	//initializing the test list for suites inherited from Mut_suite
	test_list = new ListOfTests();
	
	//int totalTestCount; 
	
	int passedTests = 0, failedTests = 0;

	populate();
	
	//following code assumes that the test suite will be 
	//executed just once per program run
	
	vector<DefaultTest *>::iterator current_test;
	
	DefaultSuite *old_suite = NULL;
	
	bool continue_testing;
	 
	old_suite = mut_control->current_suite;
	
	mut_control->current_suite = this;
	
	if(mut_control->get_mode() == MUT_TEST_LIST){
		this->list_tests();
		exit(0); //List of tests printed - return
	}
	if(mut_control->get_mode() == MUT_TEST_INFO){
		this->info();
		exit(0); //Test info printed - return	
	}
	
	current_test = this->test_list->get_list_begin();
	
	//totalTestCount = this->test_list->size();
	
	if(!this->test_list->empty()){

		//starting up the suite

		continue_testing = true;

		while(current_test != this->test_list->get_list_end() && continue_testing){

			mut_control->current_test_runner = mut_control->runfactory(this, *current_test);

			suiteExecutionStatus =+ mut_control->current_test_runner->run();

			delete mut_control->current_test_runner;

			mut_control->current_test_runner = NULL;

			if((*current_test)->statusFailed())
			{
				continue_testing = false;
				mut_control->mut_log->log_message("Test Failed \n");
				//failedTests++;
			}else{
				mut_control->mut_log->log_message("Test Executed Successfully \n");
				//passedTests++;
			}
			current_test++;
		}

	}
	
	if(suiteExecutionStatus != 0){
		mut_control->mut_log->log_message("Suite Execution Failed \n");
	}else{
		mut_control->mut_log->log_message("Suite Execution Completed Successfully \n");
	}
	
	mut_control->mut_log->close_log();
	
	return suiteExecutionStatus;
}

//timeout value should be specified in seconds and at the time of adding test to the suite
void DefaultSuite::addTest(DefaultTest *test_instance,unsigned long timeout)
{
	
	if(mut_str_in_list(mut_control->run_tests.get(),test_instance->get_test_id(),
		test_counter)){

		DefaultTest *current_test = test_instance;

		current_test->set_test_index(test_counter);

		current_test->setTimeout(timeout);

		if(current_test == NULL){
			printf("Failed to allocate memory for test %s \n",test_instance->get_test_id());
		}

		test_list->push_back(current_test);

		test_counter++;

		mut_control->mut_log->log_message("Mut Test Added \n");
	}
}

void DefaultSuite::list_tests()
{
    vector<DefaultTest *>::iterator current_test = test_list->get_list_begin();

    /* get head of tests linked list */

	printf("%s: \n", getName());
    while(current_test != test_list->get_list_end())
    {
        const char * desc = (*current_test)->get_short_description();
        printf("(%d) %s%s%s\n", (*current_test)->get_test_index(),
            (*current_test)->get_test_id(),
            desc ? ":    " : "",
            desc ? desc : "");
        current_test++;
    }

	return;
}


void DefaultSuite::info()
{

    vector<DefaultTest *>::iterator current_test = test_list->get_list_begin();
    /* get head of tests linked list */

    while(current_test != test_list->get_list_end())
    {

        printf("(%d) %s.%s\n  Short description: %s\n  Long  description: %s\n",
               (*current_test)->get_test_index(),
               getName(), (*current_test)->get_test_id(),
               (*current_test)->get_short_description());

        current_test++;
    }

	return;
}

// see if this is last in the list that's passed in
bool DefaultSuite::is_last(ListOfSuites *suite_list)
{
    bool last_testsuite_flag = false;
    DefaultSuite *suite;
	vector<DefaultSuite *>::iterator next_suite= suite_list->get_list_begin();

	/* if current suite is NOT the first test suite  */
	if (strcmp((*next_suite)->getName(), getName()) != 0)
     {
        while(next_suite!= suite_list->get_list_end() ){

			suite = *next_suite;
			next_suite++;

		}

	    /* if current suite is the last test suite */
		if (strcmp(suite->getName(), getName()) == 0)
		{
			last_testsuite_flag = true ;
		}
	 }

	return last_testsuite_flag;
}

const char* DefaultSuite::getName()
{
    return suiteName;
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
 *  numbers or both.
 */
int DefaultSuite::mut_str_in_list(const char *list, const char *str, int index)
{
	int start = 0;
	int length = 0;
	char c = ' ';
	char buf[128];

	if (list == NULL) return 1;
	while(c != '\0')
	{
		do
	    {
			c = list[start + length++];
	    } while (!(c == '\0' || c == ','));

	    if ((length - 1 == strlen(str)) &&  strncmp((char *)(list + start), str, length - 1) == 0)
	    {
	    	//change this 1 to true in typedef using generic type
	    	return 1;
	    }

	    if (index >= 0)
	    {
	    	strncpy(buf, (char *)(list + start), length - 1);
	    	if (isdigit(buf[0]))
	    	{
	    		buf[length - 1] = 0;
	    		if (atoi(buf) == index)
	    		{
	    			return 1;
	    		}
	    	}
	    }
	    start += length;
	    length = 0;
	}

	return 0;
}
