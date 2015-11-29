/*
 * mut_exception_processing.cpp
 *
 *  Created on: Feb 21, 2012
 *      Author: londhn
 */

#include "mut/mut_exception_processing.h"
#include "mut.h"
#include "mut_test_control.h"
#include "CrashHelper.h"

#include "EmcPAL_Misc.h"

#include <cstdlib>
#include <iostream>

using namespace std;

extern Mut_test_control *mut_control;

LPTOP_LEVEL_EXCEPTION_FILTER MutExceptionProcessing::pUnhandledExceptionHandler = NULL;

void MutExceptionProcessing::setHandler(){
	addUnhandledExceptionHandler();
}	

void MutExceptionProcessing::addUnhandledExceptionHandler(){
	pUnhandledExceptionHandler = SetUnhandledExceptionFilter(&MutExceptionProcessing::UnhandledExceptionHandler);
}

void MutExceptionProcessing::removeUnhandledExceptionHandler(){
	if(pUnhandledExceptionHandler != NULL){
		SetUnhandledExceptionFilter(NULL);
		pUnhandledExceptionHandler = NULL;
	}
}

long WINAPI MutExceptionProcessing::UnhandledExceptionHandler(LPEXCEPTION_POINTERS ExceptionInfo){
	
	char *pDumpFileName = "mut_dump_autogenerate.dmp";	

	mut_control->mut_log->log_message("Exception occured in current test \n");

	//report suite execution failure
	mut_control->mut_log->log_message("Test Execution Failed \n");

	EmcpalDebugBreakPoint();

	if(strcmp(mut_control->abort.get(),"dump") == 0){
		mut_control->mut_log->log_message("Control is creating dump before exit \n");
			
		mut_printf("Writing dump file to: %s \n",pDumpFileName);

		CreateDumpFile(pDumpFileName, ExceptionInfo);
			//Closing the log before exiting out the control
		mut_control->mut_log->close_log();

		exit(1);
	}else{
		return EXCEPTION_CONTINUE_SEARCH;
	}
}
