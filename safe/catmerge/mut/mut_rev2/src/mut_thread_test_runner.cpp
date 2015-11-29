/*
 * mut_thread_test_runner.cpp
 *
 *  Created on: Sep 22, 2011
 *      Author: londhn
 */
#include "mut_test_control.h"
#include "mut_thread_test_runner.h"
#include "mut/defaultsuite.h"
#include "mut/defaulttest.h"

#include "mut/mut_exception_processing.h"

#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <typeinfo>

#include "EmcPAL.h"
#include "EmcPAL_DriverShell.h"

extern Mut_test_control *mut_control;

static Mut_thread_test_runner *current_runner = NULL;
Mut_thread_test_runner::Mut_thread_test_runner(Mut_test_control *control,
												DefaultSuite *suite,
												DefaultTest *test)
:mControl(control),mSuite(suite),mTest(test){}

const char * Mut_thread_test_runner::getName(){
	return mTest->get_test_id();
}

Mut_test_control *Mut_thread_test_runner::getControl(){
	return mControl;
}

DefaultSuite    *Mut_thread_test_runner::getSuite(){
	return mSuite;
}

DefaultTest   *Mut_thread_test_runner::getTest(){
	return mTest;
}

void Mut_thread_test_runner::setTestStatus(enum Mut_test_status_e status){
	mTest->set_test_status(status);
}

int Mut_thread_test_runner::run(){

	int status; 
	EMCPAL_STATUS timeoutStatus;
	current_runner = this;
 	
	MutExceptionProcessing::setHandler();

	setTestStatus(MUT_TEST_RESULT_UKNOWN);

	char logBuffer[MUT_LOG_MAX_SIZE];
	sprintf(logBuffer,"Running test %s \n",mTest->get_test_id());
	mut_control->mut_log->log_message(logBuffer);

	
		char *policy = mut_control->abort.get();

	if(mut_control->fail_on_timeout.get()){
		
		//converting test associated time into milliseconds
		unsigned long timeout = (this->getTest()->getTimeout())*1000;

		EmcpalMonostableTimerCreate(EmcpalDriverGetCurrentClientObject(),
									&mTimer,
									"Mut_test_timer",
									mutCleanUpAndExit,
									(EMCPAL_TIMER_CONTEXT)this);

		timeoutStatus = EmcpalMonostableTimerStart(&mTimer,timeout);


		this->getTest()->execute();

		EmcpalMonostableTimerDestroy(&mTimer);
	}else{//think about how you can remove this redudancy
		
		this->getTest()->execute();

	}

	//Check if the timer is created or not. When "-failontimeout" is not specified 
	//timer will not be created


	if(mTest->get_test_status() == MUT_TEST_RESULT_UKNOWN){
		setTestStatus(MUT_TEST_PASSED);
		status = (int)mTest->get_test_status();
	}

	return status;
}

void Mut_thread_test_runner::mutCleanUpAndExit(EMCPAL_TIMER_CONTEXT context){

	//Currently its hard to tell where exactly timeout has occured (startUp/test/tearDown);
	mut_control->mut_log->log_message("Timeout occured in given test \n");

	//report suite execution failure
	mut_control->mut_log->log_message("Test Execution Failed \n");
	
	//Closing the log before exiting out the control
	mut_control->mut_log->close_log();

	exit(1);
}